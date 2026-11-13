"""Verify runtime demo-clock guards using primary engine RTTI and function bytes."""
import argparse
import struct
from pathlib import Path
from check_cs2_binary_compatibility import PeImage, parse_hex_constant


def check(engine: Path, source: Path):
    image = PeImage.read(engine)
    code = source.read_text(encoding="utf-8")
    constants = {name: parse_hex_constant(code, name) for name in (
        "kDemoEngineTimestamp", "kDemoEngineImageSize", "kDemoEngineChecksum",
        "kDemoGetFileRva", "kDemoGetTickRva")}
    if (image.timestamp, image.size_of_image, image.checksum) != tuple(constants[name] for name in (
            "kDemoEngineTimestamp", "kDemoEngineImageSize", "kDemoEngineChecksum")):
        raise ValueError("engine2 fingerprint does not match the guarded demo clock")
    pe = struct.unpack_from("<I", image.data, 0x3C)[0]
    base = struct.unpack_from("<Q", image.data, pe + 48)[0]

    def raw(rva):
        for section in image.sections:
            if section.contains_rva(rva):
                return section.raw_offset + rva - section.virtual_address
        raise ValueError(f"RVA {rva:X} is outside engine2")

    def primary_vtable(name):
        descriptor = image.raw_to_rva(image.data.index(name) - 16)
        needle = struct.pack("<I", descriptor)
        offset, matches = 0, []
        while (hit := image.data.find(needle, offset)) >= 0:
            offset = hit + 1
            col = hit - 12
            if col < 0 or struct.unpack_from("<II", image.data, col) != (1, 0):
                continue
            col_rva = image.raw_to_rva(col)
            if struct.unpack_from("<I", image.data, col + 20)[0] != col_rva:
                continue
            at = image.data.find(struct.pack("<Q", base + col_rva))
            if at >= 0: matches.append(at + 8)
        if len(matches) != 1:
            raise ValueError(f"expected one primary vtable for {name!r}, got {len(matches)}")
        return matches[0]

    for name, slot, constant in (
        (b".?AVCEngineClient@@\0", 69, "kDemoGetFileRva"),
        (b".?AVCDemoPlayer@@\0", 3, "kDemoGetTickRva"),
    ):
        table = primary_vtable(name)
        function = struct.unpack_from("<Q", image.data, table + slot * 8)[0] - base
        if function != constants[constant] or not image.is_executable_rva(function):
            raise ValueError(f"{constant}: vtable slot {slot} does not match executable target")
        body = image.data[raw(function):raw(function) + 40]
        if slot == 69 and not (body[:3] == b"\x48\x8b\x05" and body[7] == 0xC3):
            raise ValueError("GetDemoFile is no longer a read-only demo-player pointer getter")
        if slot == 3 and not (body[:9] == bytes.fromhex("40 53 48 83 ec 20 48 8b d9")
                and body[19:32] == bytes.fromhex("ff 90 c0 01 00 00 2b 83 04 02 00 00 48")):
            raise ValueError("GetDemoTick no longer calls the tick clock and subtracts the demo start tick")
        print(f"{constant}: primary RTTI slot {slot}, executable RVA 0x{function:X}, body verified")
    player_source = source.with_name("player_motion_clock.inl")
    if player_source.exists():
        pause_rva = parse_hex_constant(player_source.read_text(encoding="utf-8"), "kPlayerDemoPauseRva")
        table = primary_vtable(b".?AVCDemoPlayer@@\0")
        actual = struct.unpack_from("<Q", image.data, table + 12 * 8)[0] - base
        if actual != pause_rva or not image.is_executable_rva(actual):
            raise ValueError("player pause-query slot 12 does not match executable target")
        body = image.data[raw(actual):raw(actual) + 40]
        # Build 26598758 moved the two demo-pause flags by eight bytes; the
        # runtime calls this verified virtual method rather than reading flags.
        if body[:8] != bytes.fromhex("80 b9 38 12 00 00 00 74") or bytes.fromhex("0f b6 81 39 12 00 00 c3") not in body:
            raise ValueError("IsDemoPaused no longer matches the verified read-only query")
        print(f"player IsDemoPaused: slot 12, executable RVA 0x{actual:X}, read-only body verified")
    print("Demo clock compatibility passed (HLAE SDK slots 69/3, 64 demo ticks per second)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", required=True, type=Path)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1]
        / "native_dll/selfiestick_hlae/prop_demo_clock.inl")
    args = parser.parse_args()
    try:
        check(args.engine, args.source)
    except (ValueError, OSError) as error:
        parser.exit(1, f"FAIL: {error}\n")
