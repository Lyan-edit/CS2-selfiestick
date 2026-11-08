"""Check that user-confirmed fix5 sampling/rig sections remain unchanged."""
import argparse
import hashlib
import json
from pathlib import Path


def function_text(source: str, signature: str):
    start = source.index(signature)
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end].encode("utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
        default=Path(__file__).resolve().parents[1] / "docs/fix5-stable-sections.json",
        help="stable section hashes, or manifest.json from the complete fix5 checkpoint")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1] / "native_dll/selfiestick_hlae"
    cpp = (root / "selfiestick_hlae.cpp").read_text(encoding="utf-8")
    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    checks = manifest["protected_origin_functions"]
    for name, expected in checks.items():
        if name.endswith(".inl"):
            payload = (root / name).read_bytes()
        elif name == "ApplyPropViewFov":
            payload = function_text((root / "prop_view.inl").read_text(encoding="utf-8"), "bool ApplyPropViewFov(")
        else:
            payload = function_text(cpp, name)
        actual = hashlib.sha256(payload).hexdigest()
        if actual != expected:
            parser.exit(1, f"FAIL: confirmed fix5 section changed: {name}\n")
        print(f"unchanged: {name}")
    print(f"Confirmed fix5 preservation passed: {len(checks)} protected sections")
