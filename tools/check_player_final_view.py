"""Verify the exact-build final-view boundary without loading the game DLL."""
import argparse
import re
import struct
from pathlib import Path
from check_cs2_binary_compatibility import PeImage, parse_hex_constant, parse_patterns, find_pattern


def check(client: Path, source: Path):
    image = PeImage.read(client)
    code = source.read_text(encoding="utf-8")
    main = source.with_name("selfiestick_hlae.cpp").read_text(encoding="utf-8")
    if (image.timestamp, image.size_of_image, image.checksum) != (
            parse_hex_constant(main, "kSupportedClientTimeDateStamp"),
            parse_hex_constant(main, "kSupportedClientSizeOfImage"),
            parse_hex_constant(main, "kSupportedClientChecksum")):
        raise ValueError("client fingerprint mismatch")
    tail = parse_hex_constant(code, "kPlayerTailPatchRva")
    match = re.search(r"kPlayerTailReplay\s*\{([^}]+)\}", code)
    if match is None: raise ValueError("tail replay missing")
    expected = bytes(int(x, 0) for x in re.findall(r"0x[0-9A-Fa-f]+|\b0\b", match[1]))
    if len(expected) != 20: raise ValueError("tail must cover exactly 20 bytes of whole instructions")
    def raw(rva):
        for section in image.sections:
            if section.contains_rva(rva): return section.raw_offset + rva - section.virtual_address
        raise ValueError("RVA outside image")
    if not image.is_executable_rva(tail) or image.data[raw(tail):raw(tail)+20] != expected:
        raise ValueError("player tail instruction bytes changed")
    hits = find_pattern(image.data, parse_patterns(main)["kPatternSetupViewPatch"], limit=2)
    if len(hits) != 1: raise ValueError("early SetupView marker ambiguous")
    early = image.raw_to_rva(hits[0])
    pdata = next(s for s in image.sections if s.name == ".pdata")
    functions = []
    for at in range(pdata.raw_offset, pdata.raw_offset+pdata.raw_size-11, 12):
        start,end,_ = struct.unpack_from("<III", image.data, at)
        if start <= early < end: functions.append((start,end))
    if len(functions) != 1: raise ValueError("native SetupView unwind function ambiguous")
    start,end = functions[0]
    if not early < tail < end: raise ValueError("final callback not in the same SetupView function after the early callback")
    prologue = image.data[raw(start):raw(start)+32]
    if bytes.fromhex("53 55 56 57 41 54 41 55 41 56 41 57 48 81 ec c8 00 00 00") not in prologue:
        raise ValueError("native stack alignment/prologue changed")
    suffix = image.data[raw(tail+20):raw(end-1)+1]
    if len(suffix) != 88 or suffix[-16:] != bytes.fromhex("49 8b e3 41 5f 41 5e 41 5d 41 5c 5f 5e 5d 5b c3"):
        raise ValueError("native final-view epilogue changed")
    for copy in (bytes.fromhex("f2 0f 11 83 54 05 00 00"), bytes.fromhex("f2 0f 11 83 60 05 00 00")):
        if copy not in suffix: raise ValueError("final origin/angle history copies missing")
    print(f"Player final-view check passed: native function 0x{start:X}..0x{end:X}, early 0x{early:X}, tail 0x{tail:X}, 20-byte replay; aligned Win64 frame and final history copies verified")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", required=True, type=Path)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1]
        / "native_dll/selfiestick_hlae/player_tail_hook.inl")
    args = parser.parse_args()
    try: check(args.client, args.source)
    except (ValueError, OSError, StopIteration) as error: parser.exit(1, f"FAIL: {error}\n")
