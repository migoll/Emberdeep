"""Blockbench rig migration and animation-data bridge for the playable fighter."""

from __future__ import annotations

import argparse
import json
import math
import shutil
from pathlib import Path

import generate_fighter as fighter


MODEL_PATH = Path(__file__).resolve().parent / "fighter_v0.bbmodel"
BACKUP_PATH = Path(__file__).resolve().parent / "fighter_v0.pre_rig.bbmodel"
PARENTING_BACKUP_PATH = Path(__file__).resolve().parent / "fighter_v0.pre_equipment_parenting.bbmodel"
ANCHOR_BACKUP_PATH = Path(__file__).resolve().parent / "fighter_v0.pre_generic_anchors.bbmodel"
OUTPUT_PATH = (
    Path(__file__).resolve().parents[3]
    / "Source" / "Emberdeep" / "Private" / "Gameplay" / "FighterAnimationData.inl"
)

BONES = ("Torso", "Head", "LeftArm", "RightArm", "LeftLeg", "RightLeg", "MainHand", "OffHand")
BONE_PIVOTS_UE = {
    "Torso": (1.0, 0.0, 0.0),
    "Head": (2.0, 0.0, 12.0),
    "LeftArm": (1.0, -10.0, 5.0),
    "RightArm": (1.0, 10.0, 5.0),
    "LeftLeg": (1.0, -4.0, -5.0),
    "RightLeg": (1.0, 4.0, -5.0),
}
EXPECTED_CLIPS = ("Idle", "Walk", "BasicAttack", "HeavyAttack", "Dodge")
CLIP_SAMPLE_RATES = {
    "Idle": 18,
    "Walk": 18,
    "BasicAttack": 60,
    "HeavyAttack": 60,
    "Dodge": 24,
}


def body_bone(x: int, y: int, z: int) -> str:
    if z >= 11 and abs(y) <= 8:
        return "Head"
    if z <= -6:
        return "LeftLeg" if y < 0 else "RightLeg"
    if z <= 5 and y <= -9:
        return "LeftArm"
    if z <= 5 and y >= 9:
        return "RightArm"
    return "Torso"


def keyframe(channel: str, time: float, values: tuple[float, float, float], label: str) -> dict:
    return {
        "channel": channel,
        "data_points": [{"x": values[0], "y": values[1], "z": values[2]}],
        "uuid": fighter.stable_uuid(f"animation/key/{label}/{channel}/{time}"),
        "time": time,
        "color": -1,
        "interpolation": "linear",
    }


def starter_clips(group_ids: dict[str, str]) -> list[dict]:
    definitions = {
        "Idle": (1.0, "loop", {
            "Torso": {"position": [(0, (0, 0, 0)), (.5, (0, .35, 0)), (1, (0, 0, 0))]},
            "Head": {"rotation": [(0, (0, -3, 0)), (.5, (0, 3, 0)), (1, (0, -3, 0))]},
            "LeftArm": {"rotation": [(0, (-2, 0, -2)), (.5, (2, 0, 1)), (1, (-2, 0, -2))]},
            "RightArm": {"rotation": [(0, (2, 0, 2)), (.5, (-2, 0, -1)), (1, (2, 0, 2))]},
        }),
        "Walk": (2 / 3, "loop", {
            "Torso": {"position": [(0, (0, 0, 0)), (1 / 3, (0, .65, 0)), (2 / 3, (0, 0, 0))]},
            "LeftArm": {"rotation": [(0, (-18, 0, 0)), (1 / 3, (18, 0, 0)), (2 / 3, (-18, 0, 0))]},
            "RightArm": {"rotation": [(0, (18, 0, 0)), (1 / 3, (-18, 0, 0)), (2 / 3, (18, 0, 0))]},
            "LeftLeg": {"rotation": [(0, (18, 0, 0)), (1 / 3, (-18, 0, 0)), (2 / 3, (18, 0, 0))]},
            "RightLeg": {"rotation": [(0, (-18, 0, 0)), (1 / 3, (18, 0, 0)), (2 / 3, (-18, 0, 0))]},
        }),
        "BasicAttack": (.42, "once", {
            "Torso": {"rotation": [(0, (0, 0, 0)), (.12, (0, -12, 0)), (.28, (0, 16, 0)), (.42, (0, 0, 0))]},
            "RightArm": {"rotation": [(0, (0, 0, 0)), (.12, (-35, 0, -28)), (.28, (48, 0, 34)), (.42, (0, 0, 0))]},
            "MainHand": {"rotation": [(0, (0, 0, 0)), (.12, (-22, 0, 0)), (.28, (30, 0, 0)), (.42, (0, 0, 0))]},
        }),
        "HeavyAttack": (.72, "once", {
            "Torso": {"rotation": [(0, (0, 0, 0)), (.25, (0, -20, -4)), (.5, (0, 24, 5)), (.72, (0, 0, 0))]},
            "RightArm": {"rotation": [(0, (0, 0, 0)), (.25, (-60, 0, -42)), (.5, (72, 0, 48)), (.72, (0, 0, 0))]},
            "MainHand": {"rotation": [(0, (0, 0, 0)), (.25, (-30, 0, 0)), (.5, (42, 0, 0)), (.72, (0, 0, 0))]},
            "LeftArm": {"rotation": [(0, (0, 0, 0)), (.25, (-8, 0, 12)), (.5, (12, 0, -10)), (.72, (0, 0, 0))]},
        }),
        "Dodge": (.28, "once", {
            "Torso": {"position": [(0, (0, 0, 0)), (.10, (0, -1.0, -1.5)), (.22, (0, .25, 1.0)), (.28, (0, 0, 0))],
                      "rotation": [(0, (0, 0, 0)), (.10, (-12, 0, 0)), (.28, (0, 0, 0))]},
            "LeftArm": {"rotation": [(0, (0, 0, 0)), (.10, (18, 0, -10)), (.28, (0, 0, 0))]},
            "RightArm": {"rotation": [(0, (0, 0, 0)), (.10, (18, 0, 10)), (.28, (0, 0, 0))]},
        }),
    }
    clips = []
    for clip_name in EXPECTED_CLIPS:
        length, loop, tracks = definitions[clip_name]
        animators = {}
        for bone_name, channels in tracks.items():
            keys = []
            for channel, entries in channels.items():
                keys.extend(keyframe(channel, time, values, f"{clip_name}/{bone_name}") for time, values in entries)
            animators[group_ids[bone_name]] = {"name": bone_name, "type": "bone", "keyframes": keys}
        clips.append({
            "uuid": fighter.stable_uuid(f"animation/{clip_name}"), "name": clip_name,
            "loop": loop, "override": False, "length": length,
            "snapping": CLIP_SAMPLE_RATES[clip_name],
            "selected": clip_name == "Idle", "animators": animators,
        })
    return clips


def migrate() -> None:
    original = MODEL_PATH.read_bytes()
    model = fighter.read_bbmodel(MODEL_PATH)
    document = json.loads(original)
    if any(group.get("name") == "Torso" for group in document.get("groups", [])):
        print("Blockbench fighter already has an animation rig; leaving geometry untouched")
        return
    if not BACKUP_PATH.exists():
        shutil.copyfile(MODEL_PATH, BACKUP_PATH)

    elements_by_id = {element["uuid"]: element for element in document["elements"]}
    templates = {group["uuid"]: group for group in document.get("groups", [])}
    top = []
    for node in document["outliner"]:
        top.append({**templates.get(node.get("uuid"), {}), **node})
    part_nodes = {}
    for node in top:
        name = node.get("name")
        if name == "Body": part_nodes["Body"] = node
        elif name in ("MainHand", "Sword", "Axe"): part_nodes["Axe"] = node
        elif name in ("OffHand", "Shield"): part_nodes["Shield"] = node

    equipment_ids = set(part_nodes["Axe"].get("children", [])) | set(part_nodes["Shield"].get("children", []))
    elements = [elements_by_id[item] for item in equipment_ids]
    bone_children = {bone: [] for bone in BONES}
    palette_counts = {palette: 0 for palette in fighter.PALETTE}
    for bone in BONES[:6]:
        selected = {
            key: palette for key, palette in model.cells.items()
            if key[0] == "Body" and body_bone(key[1], key[2], key[3]) == bone
        }
        for index, cuboid in enumerate(fighter.greedy_cuboids(selected)):
            from_bb = fighter.unreal_to_blockbench(cuboid.minimum)
            to_bb = fighter.unreal_to_blockbench(cuboid.maximum)
            element_uuid = fighter.stable_uuid(f"rig/cube/{bone}/{cuboid.palette}/{cuboid.minimum}/{cuboid.maximum}/{index}")
            number = palette_counts[cuboid.palette]
            palette_counts[cuboid.palette] += 1
            bone_children[bone].append(element_uuid)
            elements.append({
                "name": f"{cuboid.palette}__{number:03d}", "box_uv": False, "render_order": "default",
                "locked": False, "allow_mirror_modeling": True, "from": from_bb, "to": to_bb,
                "autouv": 0, "color": fighter.PALETTE_ORDER[cuboid.palette] % 8,
                "origin": [(from_bb[i] + to_bb[i]) / 2 for i in range(3)],
                "faces": fighter.cube_faces(cuboid.palette), "type": "cube", "uuid": element_uuid,
            })

    group_ids = {bone: fighter.stable_uuid(f"rig/group/{bone}") for bone in BONES}
    body_template = templates[part_nodes["Body"]["uuid"]]
    rig_nodes = []
    rig_templates = []
    for index, bone in enumerate(BONES[:6]):
        group = {"name": bone, "origin": fighter.unreal_to_blockbench(BONE_PIVOTS_UE[bone]),
                 "color": index, "uuid": group_ids[bone]}
        rig_templates.append(group)
        rig_nodes.append({**group, "export": True, "isOpen": True, "locked": False,
                          "visibility": True, "autouv": 0, "children": bone_children[bone]})

    body_node = {**body_template, **part_nodes["Body"], "children": rig_nodes, "isOpen": True}
    equipment_nodes = [part_nodes["Axe"], part_nodes["Shield"]]
    equipment_templates = [templates[node["uuid"]] for node in equipment_nodes]
    group_ids["MainHand"] = part_nodes["Axe"]["uuid"]
    group_ids["OffHand"] = part_nodes["Shield"]["uuid"]
    document["elements"] = elements
    document["groups"] = [body_template, *rig_templates, *equipment_templates]
    document["outliner"] = [body_node, *equipment_nodes]
    document["animations"] = starter_clips(group_ids)
    MODEL_PATH.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")

    migrated = fighter.read_bbmodel(MODEL_PATH)
    if migrated.cells != model.cells or migrated.pivots != model.pivots or migrated.rotations != model.rotations:
        MODEL_PATH.write_bytes(original)
        raise RuntimeError("Rig migration changed fighter geometry; original restored")
    print(f"Created rig and starter animations; original backed up as {BACKUP_PATH.name}")


def parent_equipment() -> None:
    """Make Blockbench's visible hierarchy match Unreal's arm attachments."""
    original = MODEL_PATH.read_bytes()
    model_before = fighter.read_bbmodel(MODEL_PATH)
    document = json.loads(original)
    templates = {
        group.get("uuid"): group
        for group in document.get("groups", [])
        if isinstance(group, dict) and group.get("uuid")
    }

    def merged(node: object) -> dict:
        if not isinstance(node, dict):
            return {}
        return {**templates.get(node.get("uuid"), {}), **node}

    def find_group(nodes: list[object], names: tuple[str, ...]) -> dict | None:
        for node in nodes:
            group = merged(node)
            if group.get("name") in names:
                return node if isinstance(node, dict) else None
            children = group.get("children", [])
            if isinstance(children, list):
                found = find_group(children, names)
                if found is not None:
                    return found
        return None

    outliner = document.get("outliner", [])
    if not isinstance(outliner, list):
        raise ValueError("Blockbench outliner is invalid")
    right_arm = find_group(outliner, ("RightArm",))
    left_arm = find_group(outliner, ("LeftArm",))
    sword = find_group(outliner, ("MainHand", "Sword", "Axe"))
    shield = find_group(outliner, ("OffHand", "Shield"))
    if not all((right_arm, left_arm, sword, shield)):
        raise ValueError("Could not find RightArm, LeftArm, Sword/Axe, and Shield in the Blockbench rig")

    def direct_child(parent: dict, child: dict) -> bool:
        return any(
            isinstance(item, dict) and item.get("uuid") == child.get("uuid")
            for item in merged(parent).get("children", [])
        )

    if direct_child(right_arm, sword) and direct_child(left_arm, shield):
        print("Sword and Shield are already parented to their arms")
        return

    def detach(nodes: list[object], target_uuid: str) -> bool:
        for index, node in enumerate(list(nodes)):
            if isinstance(node, dict) and node.get("uuid") == target_uuid:
                nodes.pop(index)
                return True
            group = merged(node)
            children = group.get("children", [])
            if isinstance(children, list) and detach(children, target_uuid):
                if isinstance(node, dict):
                    node["children"] = children
                return True
        return False

    detach(outliner, sword["uuid"])
    detach(outliner, shield["uuid"])
    right_children = merged(right_arm).get("children", [])
    left_children = merged(left_arm).get("children", [])
    if not isinstance(right_children, list) or not isinstance(left_children, list):
        raise ValueError("Arm groups have invalid children")
    right_children.append(sword)
    left_children.append(shield)
    right_arm["children"] = right_children
    left_arm["children"] = left_children
    document["outliner"] = outliner

    if not PARENTING_BACKUP_PATH.exists():
        shutil.copyfile(MODEL_PATH, PARENTING_BACKUP_PATH)
    MODEL_PATH.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")
    try:
        model_after = fighter.read_bbmodel(MODEL_PATH)
        if model_after != model_before:
            raise RuntimeError("equipment parenting changed voxel data or rest transforms")
    except Exception:
        MODEL_PATH.write_bytes(original)
        raise
    print(f"Parented Sword to RightArm and Shield to LeftArm; backup: {PARENTING_BACKUP_PATH.name}")


def upgrade_sample_rates() -> None:
    document = json.loads(MODEL_PATH.read_text(encoding="utf-8"))
    changed = False
    for clip in document.get("animations", []):
        clip_name = clip.get("name")
        if clip_name in CLIP_SAMPLE_RATES and clip.get("snapping") != CLIP_SAMPLE_RATES[clip_name]:
            clip["snapping"] = CLIP_SAMPLE_RATES[clip_name]
            changed = True
    if changed:
        MODEL_PATH.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")
        print("Updated Blockbench clip snapping to the runtime sample rates")
    else:
        print("Blockbench clip snapping already matches the runtime sample rates")


def use_generic_anchor_names() -> None:
    original = MODEL_PATH.read_bytes()
    model_before = fighter.read_bbmodel(MODEL_PATH)
    document = json.loads(original)
    changed = False
    uuid_names: dict[str, str] = {}
    for group in document.get("groups", []):
        if group.get("name") in ("Sword", "Axe"):
            group["name"] = "MainHand"
            changed = True
        elif group.get("name") == "Shield":
            group["name"] = "OffHand"
            changed = True
        if group.get("uuid"):
            uuid_names[group["uuid"]] = group.get("name", "")
    for animation in document.get("animations", []):
        for uuid, animator in animation.get("animators", {}).items():
            if uuid_names.get(uuid) in ("MainHand", "OffHand"):
                animator["name"] = uuid_names[uuid]
    if not changed:
        print("The fighter already uses MainHand and OffHand anchor names")
        return
    if not ANCHOR_BACKUP_PATH.exists():
        shutil.copyfile(MODEL_PATH, ANCHOR_BACKUP_PATH)
    MODEL_PATH.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")
    try:
        if fighter.read_bbmodel(MODEL_PATH) != model_before:
            raise RuntimeError("generic anchor rename changed fighter geometry")
    except Exception:
        MODEL_PATH.write_bytes(original)
        raise
    print(f"Renamed animation anchors to MainHand and OffHand; backup: {ANCHOR_BACKUP_PATH.name}")


def number(value: object, context: str) -> float:
    try:
        result = float(value)
    except (TypeError, ValueError) as error:
        raise ValueError(f"{context} must be a plain numeric value (Molang is not supported in-game yet)") from error
    if not math.isfinite(result):
        raise ValueError(f"{context} must be finite")
    return result


def sample(keys: list[dict], time: float) -> tuple[float, float, float]:
    if not keys:
        return (0.0, 0.0, 0.0)
    keys = sorted(keys, key=lambda item: number(item.get("time", 0), "keyframe time"))
    before = keys[0]
    after = keys[-1]
    for candidate in keys:
        candidate_time = number(candidate.get("time", 0), "keyframe time")
        if candidate_time <= time + 1e-6:
            before = candidate
        if candidate_time >= time - 1e-6:
            after = candidate
            break
    point_before = before.get("data_points", [{}])[0]
    point_after = after.get("data_points", [{}])[0]
    a = tuple(number(point_before.get(axis, 0), "keyframe coordinate") for axis in "xyz")
    b = tuple(number(point_after.get(axis, 0), "keyframe coordinate") for axis in "xyz")
    ta = number(before.get("time", 0), "keyframe time")
    tb = number(after.get("time", 0), "keyframe time")
    if before.get("interpolation") == "step" or tb <= ta + 1e-6:
        return a
    alpha = max(0.0, min(1.0, (time - ta) / (tb - ta)))
    return tuple(a[i] + (b[i] - a[i]) * alpha for i in range(3))


def animation_bytes() -> bytes:
    document = json.loads(MODEL_PATH.read_text(encoding="utf-8"))
    names = {}
    for group in document.get("groups", []):
        name = group.get("name")
        if name in ("Sword", "Axe"):
            name = "MainHand"
        elif name == "Shield":
            name = "OffHand"
        names[group.get("uuid")] = name
    clips_by_name = {clip.get("name"): clip for clip in document.get("animations", [])}
    missing = [name for name in EXPECTED_CLIPS if name not in clips_by_name]
    if missing:
        raise ValueError(f"Missing Blockbench animation clips: {', '.join(missing)}")
    lines = ["// Generated from fighter_v0.bbmodel animations. Do not edit by hand."]
    clip_rows = []
    for clip_index, clip_name in enumerate(EXPECTED_CLIPS):
        clip = clips_by_name[clip_name]
        length = number(clip.get("length", 0), f"{clip_name} length")
        loop = clip.get("loop") == "loop"
        sample_rate = CLIP_SAMPLE_RATES[clip_name]
        frame_count = max(2, round(length * sample_rate) + (0 if loop else 1))
        symbol = "GPlayable" + clip_name + "Frames"
        lines.extend((f"static const FPlayableVoxelBonePose {symbol}[] =", "{"))
        animators = clip.get("animators", {})
        tracks_by_bone = {names.get(uuid): track for uuid, track in animators.items()}
        for frame in range(frame_count):
            time = min(length, frame / sample_rate)
            lines.append(f"\t// Frame {frame} ({time:.4f}s)")
            for bone in BONES:
                keys = tracks_by_bone.get(bone, {}).get("keyframes", [])
                rotations = [key for key in keys if key.get("channel") == "rotation"]
                positions = [key for key in keys if key.get("channel") == "position"]
                rotation = sample(rotations, time)
                quaternion = fighter.blockbench_rotation_to_unreal_quaternion(rotation)
                position_ue = fighter.blockbench_to_unreal(list(sample(positions, time)))
                translation = tuple(component * fighter.VOXEL_UNIT_CM for component in position_ue)
                lines.append("\t{ FQuat(%.9ff, %.9ff, %.9ff, %.9ff), FVector(%.4ff, %.4ff, %.4ff) }," %
                             (*quaternion, *translation))
        lines.extend(("};", ""))
        clip_rows.append((clip_name, loop, length, sample_rate, frame_count, symbol))
        lines.append(f"static constexpr int32 GPlayable{clip_name}ClipIndex = {clip_index};")
    lines.extend(("", "static const FPlayableVoxelAnimationClip GPlayableAnimationClips[] =", "{"))
    for name, loop, length, sample_rate, frame_count, symbol in clip_rows:
        lines.append(
            f'\t{{ TEXT("{name}"), {str(loop).lower()}, {length:.6f}f, '
            f'{sample_rate:.1f}f, {frame_count}, {symbol} }},'
        )
    lines.extend(("};", f"static constexpr int32 GPlayableAnimationClipCount = {len(clip_rows)};", ""))
    return "\n".join(lines).encode("utf-8")


def generate(check: bool) -> None:
    content = animation_bytes()
    if check:
        if not OUTPUT_PATH.exists() or OUTPUT_PATH.read_bytes() != content:
            raise SystemExit(f"Generated animation output is stale: {OUTPUT_PATH}")
    else:
        OUTPUT_PATH.write_bytes(content)


def set_clip_duration(clip_name: str, duration: float) -> None:
    if duration <= 0:
        raise SystemExit("Clip duration must be greater than zero")
    document = json.loads(MODEL_PATH.read_text(encoding="utf-8"))
    clip = next(
        (candidate for candidate in document.get("animations", []) if candidate.get("name") == clip_name),
        None,
    )
    if clip is None:
        raise SystemExit(f"Animation clip not found: {clip_name}")
    latest_keyframe = max(
        (
            number(keyframe.get("time", 0), f"{clip_name} keyframe time")
            for track in clip.get("animators", {}).values()
            for keyframe in track.get("keyframes", [])
        ),
        default=0.0,
    )
    if latest_keyframe > duration + 1e-6:
        raise SystemExit(
            f"{clip_name} has a keyframe at {latest_keyframe:.5f}s beyond the requested {duration:.5f}s"
        )
    clip["length"] = duration
    MODEL_PATH.write_text(json.dumps(document, separators=(",", ":")), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--migrate", action="store_true")
    parser.add_argument("--parent-equipment", action="store_true")
    parser.add_argument("--upgrade-sample-rates", action="store_true")
    parser.add_argument("--generic-anchors", action="store_true")
    parser.add_argument("--set-clip-duration", nargs=2, metavar=("CLIP", "SECONDS"))
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.migrate:
        migrate()
    if args.parent_equipment:
        parent_equipment()
    if args.upgrade_sample_rates:
        upgrade_sample_rates()
    if args.generic_anchors:
        use_generic_anchor_names()
    if args.set_clip_duration:
        set_clip_duration(args.set_clip_duration[0], float(args.set_clip_duration[1]))
    generate(args.check)
    print("Generated Unreal animation data from Blockbench clips")


if __name__ == "__main__":
    main()
