"""Reject v0.4.9 addresses under the new build fingerprint without loading CS2."""
import argparse
from contextlib import redirect_stdout
import io
from pathlib import Path
import re
import tempfile

from check_cs2_binary_compatibility import (
    PeImage, check_prop_lifecycle_virtuals, check_resolved_origin_accessor,
)
from check_player_final_view import check as check_tail
from check_player_render_clock import check as check_render
from check_prop_demo_clock import check as check_demo


def replace_constant(source, name, value):
    result, count = re.subn(
        rf"(constexpr std::uint32_t {re.escape(name)} = )0x[0-9A-Fa-f]+u;",
        rf"\g<1>0x{value:X}u;", source)
    if count != 1:
        raise ValueError(f"expected one constant: {name}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", required=True, type=Path)
    parser.add_argument("--engine", required=True, type=Path)
    args = parser.parse_args()
    source = Path(__file__).resolve().parents[1] / "native_dll/selfiestick_hlae"
    names = ("selfiestick_hlae.cpp", "player_tail_hook.inl", "player_render_clock.inl",
             "prop_demo_clock.inl", "player_motion_clock.inl")
    pristine = {name: (source / name).read_text(encoding="utf-8") for name in names}
    client = PeImage.read(args.client)
    cases = (
        ("old fingerprint", "selfiestick_hlae.cpp", "kSupportedClientTimeDateStamp", 0x6AF4A59C, "tail", "client fingerprint mismatch"),
        ("old final-view RVA", "player_tail_hook.inl", "kPlayerTailPatchRva", 0xB925E0, "tail", "player tail instruction bytes changed"),
        ("old Globals setter", "player_render_clock.inl", "kPlayerSetGlobalsRva", 0xB75C50, "render", "SetGlobals slot 11 changed"),
        ("old Globals storage", "player_render_clock.inl", "kPlayerGlobalsStorageRva", 0x2221AB8, "render", "Globals load/store"),
        ("old Demo getter", "prop_demo_clock.inl", "kDemoGetFileRva", 0x74380, "demo", "kDemoGetFileRva: vtable slot 69"),
        ("old lifecycle vtable", "selfiestick_hlae.cpp", "kPropLifecycleVtableRva", 0x1C27230, "lifecycle", "entity lifecycle primary vtable mismatch"),
        ("old origin getter", "selfiestick_hlae.cpp", "kEntityAbsOriginRva", 0x2199A0, "origin", "resolved absolute-origin accessor"),
        ("old dirty resolver", "selfiestick_hlae.cpp", "kSceneTransformResolveRva", 0x9CB410, "origin", "dirty-transform resolver target changed"),
    )
    # Keep fixtures inside the workspace, including in restricted Windows hosts
    # whose process TEMP directory cannot store generated C++ source files.
    temporary_parent = source.parents[1] / "dist"
    if not temporary_parent.resolve().is_relative_to(source.parents[1].resolve()):
        raise ValueError("temporary fixture directory must stay in the repository")
    temporary_parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="selfiestick-update-guards-", dir=temporary_parent) as temp:
        root = Path(temp)
        for label, filename, constant, old, kind, expected in cases:
            for name, text in pristine.items():
                (root / name).write_text(text, encoding="utf-8")
            fixture = replace_constant(pristine[filename], constant, old)
            if fixture == pristine[filename]:
                raise ValueError(f"negative fixture did not change {constant}")
            (root / filename).write_text(fixture, encoding="utf-8")
            error = ""
            with redirect_stdout(io.StringIO()):
                try:
                    if kind == "tail":
                        check_tail(args.client, root / "player_tail_hook.inl")
                    elif kind == "render":
                        check_render(args.client, root / "player_render_clock.inl")
                    elif kind == "demo":
                        check_demo(args.engine, root / "prop_demo_clock.inl")
                    else:
                        failures = []
                        function = check_prop_lifecycle_virtuals if kind == "lifecycle" else check_resolved_origin_accessor
                        function(client, (root / "selfiestick_hlae.cpp").read_text(encoding="utf-8"), failures)
                        error = "\n".join(failures)
                except ValueError as failure:
                    error = str(failure)
            if expected not in error:
                raise ValueError(f"{label} was not rejected for the expected reason: {error!r}")
            print(f"rejected: {label} ({constant}=0x{old:X})")
    print(f"Update guards passed: {len(cases)} invalid v0.4.9 fixtures rejected")


if __name__ == "__main__":
    main()
