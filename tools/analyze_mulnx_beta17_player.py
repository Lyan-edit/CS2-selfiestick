"""Read-only Beta17 camera comparison and illustrative timing/geometry models.

This does not load or execute MulNX, HLAE or CS2. Models describe the visible
source conditions; they are not measurements of an actual demo.
"""
import argparse
import hashlib
import json
import math
import re
from pathlib import Path

from check_cs2_binary_compatibility import PeImage, find_pattern


def source_evidence(root, relative, needles):
    path = root / relative
    raw = path.read_bytes()
    lines = raw.decode("utf-8-sig").splitlines()
    evidence = {}
    for needle in needles:
        hits = [i + 1 for i, line in enumerate(lines) if needle in line]
        if not hits:
            raise ValueError(f"expected Beta17 source condition missing: {relative}: {needle}")
        evidence[needle] = hits
    return {"path": str(path.resolve()), "sha256": hashlib.sha256(raw).hexdigest(), "lines": evidence}


def gate_model(rate, fps=144, seconds=10):
    # Equivalent to GetTime = integer demo tick / 64 and the forward-time
    # branch in HandleUpdateCSView, with AlwaysCaulate=false and successful poses.
    last_time = 0.0
    written = 0
    callbacks = fps * seconds
    for frame in range(callbacks):
        current_time = math.floor(frame * rate * 64 / fps) / 64
        if current_time > last_time or last_time - current_time > 0.015:
            written += 1
            last_time = current_time
    return {"rate": rate, "fps": fps, "seconds": seconds, "callbacks": callbacks,
            "pose_writes": written, "callbacks_without_pose_write": callbacks - written,
            "skip_percent": round(100 * (callbacks - written) / callbacks, 4)}


def analyze(mulnx, client):
    base = "source/MulNXExtensions/CS2/"
    sources = [
        (base + "Feature/View/AdvancedViewController/AdvancedViewController.cpp", [
            "currentTime > lastTime", "else return false;", "*viewSetup->pViewOrigin() = newView.position;",
            "UseBodyBones.load", "pWeapon->GetBonePos", "target->GetBonePos"]),
        (base + "Feature/View/AdvancedViewController/AdvancedViewController.hpp", [
            "AlwaysCaulate = false", "UseBodyBones = false"]),
        (base + "Feature/View/ViewSmoother/ViewSmoother.cpp", [
            "this->view.position.x +=", "*viewSetup->pViewOrigin() = out.position;",
            "this->smoothed_fov +="]),
        (base + "Feature/View/ViewSmoother/ViewSmoother.hpp", ["smooth_buffer{ 0.2f }", "fov_smooth_buffer{ 1.0f }"]),
        (base + "Support/TimeController/TimeController.cpp", ["return tick * (1.0f / 64.0f);"]),
        (base + "Intro/HookView/HookView.cpp", ["Pos_ClientModeCSNormal_CallIsPlayingDemo", "ctx->rbx"]),
        (base + "Game/tree/tree.cpp", ["unkBoneArray()", "Bones->at(index)"]),
    ]
    evidence = [source_evidence(mulnx, name, needles) for name, needles in sources]
    signature_file = mulnx / base / "Intro/Signatures.hpp"
    signature = re.search(r'Pos_ClientModeCSNormal_CallIsPlayingDemo\("([^"]+)"\)',
                          signature_file.read_text(encoding="utf-8-sig"))
    if not signature:
        raise ValueError("Beta17 view hook signature missing")
    pattern = tuple(None if token in ("?", "??") else int(token, 16) for token in signature[1].split())
    image = PeImage.read(client)
    hits = find_pattern(image.data, pattern, limit=3)
    if len(hits) != 1:
        raise ValueError(f"Beta17 hook signature has {len(hits)} matches")
    factor = 0.2
    # Illustrative whole-position smoothing after an instantaneous half-turn.
    # Both requested poses are 48 units from the same body, on opposite sides.
    positions = [48.0]
    for _ in range(4):
        positions.append(positions[-1] + (-48.0 - positions[-1]) * factor)
    return {
        "kind": "offline source audit and illustrative models; no game or foreign DLL executed",
        "source_evidence": evidence,
        "installed_client_fingerprint": {"timestamp": hex(image.timestamp), "size": hex(image.size_of_image), "checksum": hex(image.checksum)},
        "beta17_view_hook_rva": hex(image.raw_to_rva(hits[0])),
        "default_tick_gate_models": [gate_model(rate) for rate in (0.01, 0.1, 1.0)],
        "whole_pose_smoothing_model": {
            "factor": factor, "body_x": 0, "initial_camera_x": 48, "target_camera_x": -48,
            "camera_x_after_each_callback": positions,
            "inside_current_32_unit_body_guard": [abs(x) < 32 for x in positions],
            "e_fold_real_seconds": {str(fps): 1 / (-math.log(1 - factor) * fps) for fps in (30, 144, 500)},
        },
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mulnx-root", type=Path, required=True)
    parser.add_argument("--client", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = analyze(args.mulnx_root, args.client)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print("Beta17 view hook:", result["beta17_view_hook_rva"])
    for model in result["default_tick_gate_models"]:
        print(f"{model['rate']}x / 144 FPS: {model['pose_writes']} writes, "
              f"{model['callbacks_without_pose_write']} skips ({model['skip_percent']}%) over 10 seconds")
    print("Illustrative half-turn whole-pose smoothing x:", result["whole_pose_smoothing_model"]["camera_x_after_each_callback"])
    print("Source hashes, locations and model assumptions:", args.output)
