"""Generate independently editable voxel equipment and Unreal runtime data."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import math
import sys


REPO_ROOT = Path(__file__).resolve().parents[2]
FIGHTER_DIR = REPO_ROOT / "SourceAssets" / "Characters" / "Fighter"
sys.path.insert(0, str(FIGHTER_DIR))
import generate_fighter as fighter  # noqa: E402


ASSET_DIR = Path(__file__).resolve().parent
WEAPON_DIR = ASSET_DIR / "Weapons"
OFFHAND_DIR = ASSET_DIR / "OffHands"
CPP_OUTPUT = REPO_ROOT / "Source" / "Emberdeep" / "Private" / "Gameplay" / "FighterEquipmentData.inl"

WEAPONS = {
    "NotchedIronSword": WEAPON_DIR / "notched_iron_sword.bbmodel",
    "NotchedIronAxe": WEAPON_DIR / "notched_iron_axe.bbmodel",
    "BonecarverAxe": WEAPON_DIR / "bonecarver_axe.bbmodel",
    "WardenCleaver": WEAPON_DIR / "warden_cleaver.bbmodel",
    "EmberdeepOath": WEAPON_DIR / "emberdeep_oath.bbmodel",
}
SHIELD_PATH = OFFHAND_DIR / "fighter_shield_v0.bbmodel"


@dataclass(frozen=True)
class EquipmentModel:
    part: str
    cells: dict[tuple[int, int, int], str]


class EquipmentBuilder:
    def __init__(self, part: str = "Axe") -> None:
        self.part = part
        self.cells: dict[tuple[int, int, int], str] = {}

    def box(self, palette: str, minimum: tuple[int, int, int], maximum: tuple[int, int, int]) -> None:
        for z in range(minimum[2], maximum[2] + 1):
            for y in range(minimum[1], maximum[1] + 1):
                for x in range(minimum[0], maximum[0] + 1):
                    self.cells[(x, y, z)] = palette

    def model(self) -> EquipmentModel:
        return EquipmentModel(self.part, dict(self.cells))


def notched_iron_axe() -> EquipmentModel:
    b = EquipmentBuilder()
    b.box("Wood", (-1, -1, -4), (1, 1, 13))
    b.box("Hide", (-2, -2, -4), (2, 2, 2))
    b.box("Steel", (-1, -5, 10), (1, 5, 15))
    b.box("Steel", (-1, -7, 12), (1, -5, 14))
    b.box("Ash", (0, -8, 12), (0, -7, 14))
    b.box("Night", (-2, 4, 11), (2, 5, 14))
    return b.model()


def notched_iron_sword() -> EquipmentModel:
    b = EquipmentBuilder()
    b.box("Wood", (-1, -1, -5), (1, 1, 5))
    b.box("Hide", (-2, -2, -4), (2, 2, 2))
    b.box("Brass", (-2, -2, -6), (2, 2, -4))
    b.box("Brass", (-1, -6, 4), (1, 6, 6))
    b.box("Steel", (-1, -2, 7), (1, 2, 23))
    b.box("Ash", (2, -2, 8), (2, 2, 22))
    b.box("Steel", (-1, -1, 24), (1, 1, 25))
    return b.model()


def bonecarver_axe() -> EquipmentModel:
    b = EquipmentBuilder()
    b.box("Wood", (-1, -1, -4), (1, 1, 14))
    b.box("Hide", (-2, -2, -4), (2, 2, 3))
    b.box("Bone", (-1, -4, 11), (1, 5, 16))
    b.box("Bone", (-1, -7, 13), (1, -4, 17))
    b.box("Ash", (0, -8, 14), (0, -7, 17))
    b.box("Fur", (-2, 3, 10), (2, 5, 15))
    return b.model()


def warden_cleaver() -> EquipmentModel:
    b = EquipmentBuilder()
    b.box("Hide", (-2, -2, -4), (2, 2, 4))
    b.box("Ash", (-1, -4, 3), (1, 4, 5))
    b.box("Steel", (-1, -4, 5), (1, 4, 18))
    b.box("Steel", (-1, -3, 18), (1, 2, 21))
    b.box("Ash", (0, -5, 7), (0, -4, 18))
    b.box("Night", (-2, 3, 8), (2, 4, 17))
    return b.model()


def emberdeep_oath() -> EquipmentModel:
    b = EquipmentBuilder()
    b.box("Hide", (-2, -2, -5), (2, 2, 4))
    b.box("Bone", (-1, -5, 3), (1, 5, 5))
    b.box("Ash", (-1, -2, 5), (1, 2, 23))
    b.box("Bone", (0, -1, 8), (0, 1, 21))
    b.box("Steel", (-1, -3, 21), (1, 3, 24))
    b.box("Bone", (0, 0, 24), (0, 0, 27))
    return b.model()


def shield_from_fighter() -> EquipmentModel:
    fighter_model = fighter.read_bbmodel(fighter.BLOCKBENCH_OUTPUT)
    return EquipmentModel(
        "Shield",
        {(x, y, z): palette for (part, x, y, z), palette in fighter_model.cells.items() if part == "Shield"},
    )


def write_bbmodel(model: EquipmentModel, identifier: str) -> bytes:
    source = {(model.part, x, y, z): palette for (x, y, z), palette in model.cells.items()}
    cuboids = fighter.greedy_cuboids(source)
    children: list[str] = []
    elements: list[dict[str, object]] = []
    counts = {palette: 0 for palette in fighter.PALETTE}
    for index, cuboid in enumerate(cuboids):
        element_uuid = fighter.stable_uuid(
            f"equipment/{identifier}/{cuboid.palette}/{cuboid.minimum}/{cuboid.maximum}/{index}"
        )
        number = counts[cuboid.palette]
        counts[cuboid.palette] += 1
        children.append(element_uuid)
        from_bb = fighter.unreal_to_blockbench(cuboid.minimum)
        to_bb = fighter.unreal_to_blockbench(cuboid.maximum)
        elements.append({
            "name": f"{cuboid.palette}__{number:03d}", "box_uv": False, "render_order": "default",
            "locked": False, "allow_mirror_modeling": True, "from": from_bb, "to": to_bb,
            "autouv": 0, "color": fighter.PALETTE_ORDER[cuboid.palette] % 8,
            "origin": [(from_bb[i] + to_bb[i]) / 2 for i in range(3)],
            "faces": fighter.cube_faces(cuboid.palette), "type": "cube", "uuid": element_uuid,
        })
    group_uuid = fighter.stable_uuid(f"equipment/{identifier}/group")
    group = {"name": "Equipment", "origin": [0, 0, 0], "color": 0, "uuid": group_uuid}
    document = {
        "meta": {"format_version": "5.0", "model_format": "free", "box_uv": False},
        "name": identifier, "model_identifier": identifier, "resolution": {"width": 144, "height": 16},
        "elements": elements, "groups": [group],
        "outliner": [{**group, "export": True, "isOpen": True, "locked": False,
                      "visibility": True, "autouv": 0, "children": children}],
        "textures": [{
            "name": "emberdeep_equipment_palette.png", "folder": "", "namespace": "", "id": "0",
            "particle": False, "render_mode": "default", "render_sides": "auto", "frame_time": 1,
            "frame_order_type": "loop", "frame_order": "", "frame_interpolate": False,
            "visible": True, "internal": True, "saved": True,
            "uuid": fighter.stable_uuid(f"equipment/{identifier}/texture"),
            "source": fighter.palette_texture_data_url(),
        }],
    }
    return (json.dumps(document, indent=2) + "\n").encode("utf-8")


def require_grid(value: object, context: str) -> int:
    if not isinstance(value, (int, float)) or not math.isfinite(value) or abs(value - round(value)) > 0.0001:
        raise ValueError(f"{context} must stay on the whole 4 cm Blockbench grid")
    return int(round(value))


def read_bbmodel(path: Path, part: str) -> EquipmentModel:
    document = json.loads(path.read_text(encoding="utf-8"))
    elements = {element.get("uuid"): element for element in document.get("elements", [])}
    groups = {group.get("uuid"): group for group in document.get("groups", [])}
    outliner = document.get("outliner", [])
    if len(outliner) != 1 or not isinstance(outliner[0], dict):
        raise ValueError(f"{path.name} must have one Equipment group")
    group = {**groups.get(outliner[0].get("uuid"), {}), **outliner[0]}
    if group.get("name") != "Equipment":
        raise ValueError(f"{path.name} group must remain named Equipment")
    cells: dict[tuple[int, int, int], str] = {}
    for child in group.get("children", []):
        if not isinstance(child, str) or child not in elements:
            raise ValueError(f"{path.name} contains a nested or missing element")
        element = elements[child]
        if element.get("rotation", [0, 0, 0]) != [0, 0, 0]:
            raise ValueError(f"Rotate the MainHand anchor, not individual cubes in {path.name}")
        palette = str(element.get("name", "")).split("__", 1)[0]
        if palette not in fighter.PALETTE:
            raise ValueError(f"{element.get('name')} needs an Emberdeep palette prefix")
        from_bb = [require_grid(v, f"{element.get('name')} from") for v in element.get("from", [])]
        to_bb = [require_grid(v, f"{element.get('name')} to") for v in element.get("to", [])]
        if len(from_bb) != 3 or len(to_bb) != 3:
            raise ValueError(f"{element.get('name')} must have three coordinates")
        minimum = fighter.blockbench_to_unreal(from_bb)
        maximum = fighter.blockbench_to_unreal(to_bb)
        for z in range(int(minimum[2]), int(maximum[2])):
            for y in range(int(minimum[1]), int(maximum[1])):
                for x in range(int(minimum[0]), int(maximum[0])):
                    cells[(x, y, z)] = palette
    return EquipmentModel(part, cells)


def visible(model: EquipmentModel) -> list[fighter.VoxelCell]:
    source = {(model.part, x, y, z): palette for (x, y, z), palette in model.cells.items()}
    return fighter.visible_cells(source)


def write_cpp(models: dict[str, EquipmentModel], shield: EquipmentModel) -> bytes:
    lines = ["// Generated by SourceAssets/Equipment/generate_equipment.py. Do not edit by hand.", ""]
    rows = []
    for definition_id, model in models.items():
        symbol = "G" + definition_id + "Cells"
        cells = visible(model)
        lines.extend((f"static const FThorgrimVoxelCell {symbol}[] =", "{"))
        for cell in cells:
            lines.append(f"\t{{ EThorgrimPart::Axe, EThorgrimPalette::{cell.palette}, {cell.x}, {cell.y}, {cell.z} }},")
        lines.extend(("};", ""))
        rows.append((definition_id, symbol, len(cells)))
    shield_cells = visible(shield)
    lines.extend(("static const FThorgrimVoxelCell GDefaultOffHandCells[] =", "{"))
    for cell in shield_cells:
        lines.append(f"\t{{ EThorgrimPart::Shield, EThorgrimPalette::{cell.palette}, {cell.x}, {cell.y}, {cell.z} }},")
    lines.extend(("};", "", "static const FPlayableEquipmentVisual GMainHandEquipmentVisuals[] =", "{"))
    for definition_id, symbol, count in rows:
        lines.append(f'\t{{ TEXT("{definition_id}"), {symbol}, {count} }},')
    lines.extend(("};", f"static constexpr int32 GMainHandEquipmentVisualCount = {len(rows)};",
                  "static const FPlayableEquipmentVisual GDefaultOffHandVisual =",
                  f'\t{{ TEXT("FighterShieldV0"), GDefaultOffHandCells, {len(shield_cells)} }};', ""))
    return "\n".join(lines).encode("utf-8")


def bootstrap() -> None:
    definitions = {
        "NotchedIronSword": notched_iron_sword(),
        "NotchedIronAxe": notched_iron_axe(), "BonecarverAxe": bonecarver_axe(),
        "WardenCleaver": warden_cleaver(), "EmberdeepOath": emberdeep_oath(),
    }
    for definition_id, path in WEAPONS.items():
        if not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(write_bbmodel(definitions[definition_id], definition_id))
    if not SHIELD_PATH.exists():
        SHIELD_PATH.parent.mkdir(parents=True, exist_ok=True)
        SHIELD_PATH.write_bytes(write_bbmodel(shield_from_fighter(), "FighterShieldV0"))


def generate(check: bool) -> None:
    models = {definition_id: read_bbmodel(path, "Axe") for definition_id, path in WEAPONS.items()}
    shield = read_bbmodel(SHIELD_PATH, "Shield")
    content = write_cpp(models, shield)
    if check:
        if not CPP_OUTPUT.exists() or CPP_OUTPUT.read_bytes() != content:
            raise SystemExit(f"Generated equipment output is stale: {CPP_OUTPUT}")
    else:
        CPP_OUTPUT.write_bytes(content)
    print("Generated equipment visuals: " + ", ".join(
        f"{name}={len(visible(model))}" for name, model in models.items()
    ) + f", Shield={len(visible(shield))}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bootstrap", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.bootstrap:
        bootstrap()
    generate(args.check)


if __name__ == "__main__":
    main()
