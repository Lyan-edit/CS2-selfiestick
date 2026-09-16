#!/usr/bin/env python3
"""Verify SelfieStick signatures against installed CS2 PE images."""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    characteristics: int

    def contains_rva(self, rva: int) -> bool:
        return self.virtual_address <= rva < self.virtual_address + max(self.virtual_size, self.raw_size)


@dataclass(frozen=True)
class PeImage:
    data: bytes
    timestamp: int
    size_of_image: int
    checksum: int
    sections: tuple[Section, ...]

    @classmethod
    def read(cls, path: Path) -> "PeImage":
        data = path.read_bytes()
        if len(data) < 0x100 or data[:2] != b"MZ":
            raise ValueError(f"{path} is not a valid PE image")
        pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe_offset : pe_offset + 4] != b"PE\0\0":
            raise ValueError(f"{path} has no PE signature")
        section_count = struct.unpack_from("<H", data, pe_offset + 6)[0]
        timestamp = struct.unpack_from("<I", data, pe_offset + 8)[0]
        optional_size = struct.unpack_from("<H", data, pe_offset + 20)[0]
        optional_offset = pe_offset + 24
        size_of_image = struct.unpack_from("<I", data, optional_offset + 56)[0]
        checksum = struct.unpack_from("<I", data, optional_offset + 64)[0]
        section_offset = optional_offset + optional_size
        sections: list[Section] = []
        for index in range(section_count):
            offset = section_offset + index * 40
            raw_name = data[offset : offset + 8].split(b"\0", 1)[0]
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, offset + 8)
            characteristics = struct.unpack_from("<I", data, offset + 36)[0]
            sections.append(
                Section(
                    raw_name.decode("ascii", errors="replace"),
                    virtual_address,
                    virtual_size,
                    raw_offset,
                    raw_size,
                    characteristics,
                )
            )
        return cls(data, timestamp, size_of_image, checksum, tuple(sections))

    def raw_to_rva(self, raw_offset: int) -> int:
        for section in self.sections:
            if section.raw_offset <= raw_offset < section.raw_offset + section.raw_size:
                return section.virtual_address + raw_offset - section.raw_offset
        raise ValueError(f"raw offset 0x{raw_offset:X} is outside PE sections")

    def is_executable_rva(self, rva: int) -> bool:
        return any(section.contains_rva(rva) and section.characteristics & 0x20000000 for section in self.sections)


def parse_patterns(source: str) -> dict[str, tuple[int | None, ...]]:
    expressions = re.findall(r'constexpr char (kPattern\w+)\[\] = "([^"]+)";', source)
    return {
        name: tuple(None if token in {"?", "??"} else int(token, 16) for token in expression.split())
        for name, expression in expressions
    }


def parse_hex_constant(source: str, name: str) -> int:
    match = re.search(rf"constexpr std::uint32_t {re.escape(name)} = 0x([0-9A-Fa-f]+)u;", source)
    if match is None:
        raise ValueError(f"missing source constant {name}")
    return int(match.group(1), 16)


def find_pattern(data: bytes, pattern: tuple[int | None, ...], limit: int = 3) -> list[int]:
    matches: list[int] = []
    concrete = next((index for index, value in enumerate(pattern) if value is not None), None)
    if concrete is None:
        return matches
    needle = bytes((pattern[concrete],))
    cursor = 0
    while len(matches) < limit:
        hit = data.find(needle, cursor)
        if hit < 0:
            break
        start = hit - concrete
        if start >= 0 and start + len(pattern) <= len(data):
            if all(expected is None or data[start + index] == expected for index, expected in enumerate(pattern)):
                matches.append(start)
        cursor = hit + 1
    return matches


def relative_call_target_rva(image: PeImage, match_offset: int, call_offset: int) -> int:
    instruction_offset = match_offset + call_offset
    if image.data[instruction_offset] != 0xE8:
        raise ValueError(f"expected CALL at pattern +0x{call_offset:X}")
    displacement = struct.unpack_from("<i", image.data, instruction_offset + 1)[0]
    return image.raw_to_rva(instruction_offset) + 5 + displacement


def check_call(image: PeImage, match_offset: int, call_offset: int, label: str, failures: list[str]) -> None:
    try:
        target_rva = relative_call_target_rva(image, match_offset, call_offset)
        if not image.is_executable_rva(target_rva):
            failures.append(f"{label} target RVA 0x{target_rva:X} is not executable")
    except ValueError as error:
        failures.append(f"{label}: {error}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--client", required=True, type=Path, help="Path to game/csgo/bin/win64/client.dll")
    parser.add_argument("--schema", required=True, type=Path, help="Path to game/bin/win64/schemasystem.dll")
    parser.add_argument(
        "--source",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "native_dll" / "selfiestick_hlae" / "selfiestick_hlae.cpp",
    )
    args = parser.parse_args()

    source = args.source.read_text(encoding="utf-8")
    patterns = parse_patterns(source)
    client = PeImage.read(args.client)
    schema = PeImage.read(args.schema)
    failures: list[str] = []

    expected_fingerprint = (
        parse_hex_constant(source, "kSupportedClientTimeDateStamp"),
        parse_hex_constant(source, "kSupportedClientSizeOfImage"),
        parse_hex_constant(source, "kSupportedClientChecksum"),
    )
    actual_fingerprint = (client.timestamp, client.size_of_image, client.checksum)
    print(
        "client fingerprint "
        f"timestamp=0x{client.timestamp:08X} image=0x{client.size_of_image:08X} checksum=0x{client.checksum:08X}"
    )
    if actual_fingerprint != expected_fingerprint:
        failures.append(
            "client fingerprint differs from the build-gated schema fallback: "
            f"expected {expected_fingerprint!r}, got {actual_fingerprint!r}"
        )

    requirements = (
        (client, "kPatternEntityList", 1, 1),
        (client, "kPatternGetEntityFromIndex", 1, 1),
        (client, "kPatternGetSplitScreenPlayer", 1, 8),
        (client, "kPatternAttachmentAccess", 1, 1),
        (client, "kPatternSetupViewPatch", 1, 1),
        (schema, "kPatternSchemaSystem", 1, 1),
        (schema, "kPatternSchemaQualifiedClassLookup", 1, 1),
    )
    resolved: dict[str, list[int]] = {}
    for image, name, minimum, maximum in requirements:
        matches = find_pattern(image.data, patterns[name], limit=maximum + 1)
        resolved[name] = matches
        offsets = ",".join(f"0x{offset:X}" for offset in matches) or "none"
        print(f"{name}: matches={len(matches)} raw={offsets}")
        if not minimum <= len(matches) <= maximum:
            failures.append(f"{name} expected {minimum}..{maximum} matches, found {len(matches)}")

    attachment = resolved.get("kPatternAttachmentAccess", [])
    if len(attachment) == 1:
        check_call(client, attachment[0], 0, "LookupAttachment", failures)
        check_call(client, attachment[0], 23, "GetAttachment", failures)

    setup_view = resolved.get("kPatternSetupViewPatch", [])
    if len(setup_view) == 1:
        check_call(client, setup_view[0], 12, "SetUpView helper", failures)
        context_start = max(0, setup_view[0] - 0x100)
        r14_origin_access = b"\x49\x8D\x96\xA0\x04\x00\x00"
        if r14_origin_access not in client.data[context_start : setup_view[0]]:
            failures.append("SetUpView context no longer proves CViewSetup origin at [R14+0x4A0]")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        return 1

    print("CS2 binary compatibility check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
