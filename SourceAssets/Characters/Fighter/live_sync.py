"""Watch Emberdeep Blockbench sources and export hot-reloadable runtime JSON."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import sys
import time

import fighter_animation_bridge as animation
import generate_fighter as fighter


REPO_ROOT = Path(__file__).resolve().parents[3]
EQUIPMENT_DIR = REPO_ROOT / "SourceAssets" / "Equipment"
sys.path.insert(0, str(EQUIPMENT_DIR))
import generate_equipment as equipment  # noqa: E402

OUTPUT_DIR = REPO_ROOT / "Intermediate" / "LiveSync"
OUTPUT_PATH = OUTPUT_DIR / "fighter_runtime.json"
LOCK_PATH = OUTPUT_DIR / "fighter_live_sync.lock"


def animation_rows() -> list[dict[str, object]]:
    document = json.loads(animation.MODEL_PATH.read_text(encoding="utf-8"))
    bone_names: dict[str, str] = {}
    for group in document.get("groups", []):
        name = group.get("name")
        if name in ("Sword", "Axe"):
            name = "MainHand"
        elif name == "Shield":
            name = "OffHand"
        bone_names[str(group.get("uuid"))] = str(name)

    clips_by_name = {clip.get("name"): clip for clip in document.get("animations", [])}
    rows: list[dict[str, object]] = []
    for clip_name in animation.EXPECTED_CLIPS:
        clip = clips_by_name[clip_name]
        length = animation.number(clip.get("length", 0), f"{clip_name} length")
        loop = clip.get("loop") == "loop"
        sample_rate = animation.CLIP_SAMPLE_RATES[clip_name]
        frame_count = max(2, round(length * sample_rate) + (0 if loop else 1))
        tracks_by_bone = {
            bone_names.get(str(uuid)): track for uuid, track in clip.get("animators", {}).items()
        }
        frames: list[list[list[float]]] = []
        for frame_index in range(frame_count):
            sample_time = min(length, frame_index / sample_rate)
            poses: list[list[float]] = []
            for bone in animation.BONES:
                keys = tracks_by_bone.get(bone, {}).get("keyframes", [])
                rotation = animation.sample(
                    [key for key in keys if key.get("channel") == "rotation"], sample_time
                )
                position = animation.sample(
                    [key for key in keys if key.get("channel") == "position"], sample_time
                )
                quaternion = fighter.blockbench_rotation_to_unreal_quaternion(rotation)
                position_ue = fighter.blockbench_to_unreal(list(position))
                translation = [component * fighter.VOXEL_UNIT_CM for component in position_ue]
                poses.append([*quaternion, *translation])
            frames.append(poses)
        rows.append({
            "name": clip_name,
            "loop": loop,
            "duration": length,
            "sample_rate": sample_rate,
            "frames": frames,
        })
    return rows


def export_runtime_data() -> None:
    fighter_model = fighter.read_bbmodel(fighter.BLOCKBENCH_OUTPUT)
    fighter_cells = [
        [cell.palette, cell.x, cell.y, cell.z]
        for cell in fighter.visible_cells(fighter_model.cells)
        if cell.part == "Body"
    ]
    weapon_rows: dict[str, list[list[object]]] = {}
    for definition_id, path in equipment.WEAPONS.items():
        model = equipment.read_bbmodel(path, "Axe")
        weapon_rows[definition_id] = [
            [cell.palette, cell.x, cell.y, cell.z] for cell in equipment.visible(model)
        ]
    shield_model = equipment.read_bbmodel(equipment.SHIELD_PATH, "Shield")
    shield_rows = [
        [cell.palette, cell.x, cell.y, cell.z] for cell in equipment.visible(shield_model)
    ]

    payload = {
        "version": 1,
        "fighter": {
            "cells": fighter_cells,
            "main_hand_pivot": [value * fighter.VOXEL_UNIT_CM for value in fighter_model.pivots["Axe"]],
            "off_hand_pivot": [value * fighter.VOXEL_UNIT_CM for value in fighter_model.pivots["Shield"]],
            "main_hand_rotation": fighter.blockbench_rotation_to_unreal_quaternion(
                fighter_model.rotations["Axe"]
            ),
            "off_hand_rotation": fighter.blockbench_rotation_to_unreal_quaternion(
                fighter_model.rotations["Shield"]
            ),
        },
        "weapons": weapon_rows,
        "shield": shield_rows,
        "animations": animation_rows(),
    }
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    temporary_path = OUTPUT_PATH.with_suffix(".tmp")
    temporary_path.write_text(json.dumps(payload, separators=(",", ":")), encoding="utf-8")
    os.replace(temporary_path, OUTPUT_PATH)
    print(
        f"[{time.strftime('%H:%M:%S')}] Live sync ready: "
        f"fighter={len(fighter_cells)}, weapons={len(weapon_rows)}, animations={len(payload['animations'])}",
        flush=True,
    )


def source_paths() -> list[Path]:
    return [fighter.BLOCKBENCH_OUTPUT, *equipment.WEAPONS.values(), equipment.SHIELD_PATH]


def source_signature() -> tuple[int, ...]:
    return tuple(path.stat().st_mtime_ns for path in source_paths())


def watch() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    lock_file = LOCK_PATH.open("a+b")
    if sys.platform == "win32":
        import msvcrt

        try:
            lock_file.seek(0, os.SEEK_END)
            if lock_file.tell() == 0:
                lock_file.write(b"\0")
                lock_file.flush()
            lock_file.seek(0)
            msvcrt.locking(lock_file.fileno(), msvcrt.LK_NBLCK, 1)
        except OSError:
            raise SystemExit("Emberdeep Live Sync is already running")

    last_signature: tuple[int, ...] | None = None
    print("Emberdeep Live Sync is watching Blockbench files. Save with Ctrl+Alt+S to update the game.", flush=True)
    while True:
        try:
            signature = source_signature()
            if signature != last_signature:
                export_runtime_data()
                last_signature = signature
        except Exception as error:
            print(f"[{time.strftime('%H:%M:%S')}] Live sync failed: {error}", flush=True)
        time.sleep(0.25)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--once", action="store_true")
    args = parser.parse_args()
    if args.once:
        export_runtime_data()
    else:
        watch()


if __name__ == "__main__":
    main()
