"""Check the exact client Globals storage using primary RTTI and SetGlobals code.

The field layout is taken from HLAE v2.193.1 MirvTime.cpp: framecount +4,
interpolation_amount +14*4. No DLL is loaded or method called by this check.
"""
import argparse
import struct
from pathlib import Path
from check_cs2_binary_compatibility import PeImage, parse_hex_constant


def check(client, source):
    p = PeImage.read(client)
    code = source.read_text(encoding="utf-8")
    main = source.with_name("selfiestick_hlae.cpp").read_text(encoding="utf-8")
    if (p.timestamp, p.size_of_image, p.checksum) != tuple(parse_hex_constant(main, n) for n in (
            "kSupportedClientTimeDateStamp", "kSupportedClientSizeOfImage", "kSupportedClientChecksum")):
        raise ValueError("client fingerprint mismatch")
    setter = parse_hex_constant(code, "kPlayerSetGlobalsRva")
    storage = parse_hex_constant(code, "kPlayerGlobalsStorageRva")
    fallback = parse_hex_constant(code, "kPlayerDefaultGlobalsRva")
    base = struct.unpack_from("<Q", p.data, struct.unpack_from("<I", p.data, 0x3C)[0] + 48)[0]
    def raw(rva):
        s = next(s for s in p.sections if s.contains_rva(rva))
        return s.raw_offset + rva - s.virtual_address
    descriptor = p.raw_to_rva(p.data.index(b".?AVCSource2Client@@\0") - 16)
    cursor = 0
    tables = set()
    while (at := p.data.find(struct.pack("<I", descriptor), cursor)) >= 0:
        cursor = at + 1
        col = at - 12
        if col < 0: continue
        fields = struct.unpack_from("<6I", p.data, col)
        if fields[:3] != (1, 0, 0): continue
        try: col_rva = p.raw_to_rva(col)
        except ValueError: continue
        if fields[5] != col_rva: continue
        ref = p.data.find(struct.pack("<Q", base + col_rva))
        if ref >= 0: tables.add(ref + 8)
    if len(tables) != 1: raise ValueError("primary Source2Client vtable ambiguous")
    target = struct.unpack_from("<Q", p.data, next(iter(tables)) + 11 * 8)[0] - base
    if target != setter or not p.is_executable_rva(setter): raise ValueError("SetGlobals slot 11 changed")
    body = p.data[raw(setter):raw(setter) + 49]
    expected = bytes.fromhex("48 8b 0d 69 d3 6a 01 4c 8d 05 12 d4 6a 01 48 85 d2 48 8d 05 08 fb f2 ff 49 0f 44 d0 48 89 15 4d d3 6a 01 48 89 42 28 33 c0 49 3b c8 48 0f 45 c1 c3")
    if body != expected: raise ValueError("SetGlobals native body changed")
    if setter + 7 + struct.unpack_from("<i", body, 3)[0] != storage or setter + 35 + struct.unpack_from("<i", body, 31)[0] != storage:
        raise ValueError("Globals load/store do not identify the same storage")
    if setter + 14 + struct.unpack_from("<i", body, 10)[0] != fallback:
        raise ValueError("default Globals pointer changed")
    if p.is_executable_rva(storage) or p.is_executable_rva(fallback): raise ValueError("Globals address is executable")
    print(f"Player render clock verified: SetGlobals[11]=0x{setter:X}, storage=0x{storage:X}, fallback=0x{fallback:X}; HLAE framecount/interpolation layout")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", required=True, type=Path)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1] / "native_dll/selfiestick_hlae/player_render_clock.inl")
    args = parser.parse_args()
    try: check(args.client, args.source)
    except (ValueError, OSError, StopIteration, struct.error) as e: parser.exit(1, f"FAIL: {e}\n")
