"""Generate Emberdeep's low-density fighter comparison character.

The fighter keeps the project's fundamental 4 cm lattice, but its silhouette is
authored mainly in 2x2-cell modules. Large uninterrupted palette regions make it
read like the simpler gameplay-scale characters in the approved combat reference.
"""

from __future__ import annotations

from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import argparse
import base64
import json
import math
import uuid

from PIL import Image, ImageDraw


REPO_ROOT = Path(__file__).resolve().parents[3]
ASSET_DIR = Path(__file__).resolve().parent
CPP_OUTPUT = REPO_ROOT / "Source" / "Emberdeep" / "Private" / "Gameplay" / "FighterVoxelData.inl"
PREVIEW_OUTPUT = ASSET_DIR / "fighter_v0_preview.png"
BLOCKBENCH_OUTPUT = ASSET_DIR / "fighter_v0.bbmodel"

VOXEL_UNIT_CM = 4.0
DESIGN_MODULE_CELLS = 2

PALETTE = {
    "Night": "#18202B",
    "Steel": "#555A60",
    "Ash": "#777572",
    "Bone": "#D2C2A2",
    "Hide": "#8A5F3C",
    "Fur": "#49372D",
    "Skin": "#B87454",
    "Wood": "#4E3525",
    "Cloth": "#243442",
}

MATERIAL_SHADES = {
    "Night": ("#121922", "#18202B", "#1F2A36"),
    "Steel": ("#44494F", "#555A60", "#70777E"),
    "Ash": ("#605D59", "#777572", "#8F8B84"),
    "Bone": ("#AE9C7C", "#D2C2A2", "#E4D5B6"),
    "Hide": ("#69462D", "#8A5F3C", "#A07045"),
    "Fur": ("#362720", "#49372D", "#5E4432"),
    "Skin": ("#9B5A42", "#B87454", "#CC8966"),
    "Wood": ("#3C271D", "#4E3525", "#60412A"),
    "Cloth": ("#1C2833", "#243442", "#2E414E"),
}

# Equipment is kept as separate rigid clusters for the existing attack proof.
PART_PIVOT_CELLS = {
    "Body": (0, 0, 0),
    "Axe": (1, 10, -3),  # Sword compatibility slot.
    "Shield": (1, -10, -2),
}

PART_ROTATION_DEGREES = {
    "Body": (0.0, 0.0, 0.0),
    "Axe": (0.0, 0.0, 0.0),
    "Shield": (0.0, 90.0, 0.0),
}

PART_DISPLAY_NAMES = {
    "Body": "Body",
    "Axe": "MainHand",
    "Shield": "OffHand",
}

PART_DISPLAY_ALIASES = {
    "Body": ("Body",),
    "Axe": ("MainHand", "Sword", "Axe"),
    "Shield": ("OffHand", "Shield"),
}

PART_ORDER = {name: index for index, name in enumerate(PART_PIVOT_CELLS)}
PALETTE_ORDER = {name: index for index, name in enumerate(PALETTE)}
NEIGHBOURS = ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))


@dataclass(frozen=True)
class VoxelCell:
    part: str
    palette: str
    x: int
    y: int
    z: int


@dataclass(frozen=True)
class AuthoringCuboid:
    part: str
    palette: str
    minimum: tuple[int, int, int]
    maximum: tuple[int, int, int]


@dataclass(frozen=True)
class FighterModel:
    cells: dict[tuple[str, int, int, int], str]
    pivots: dict[str, tuple[int, int, int]]
    rotations: dict[str, tuple[float, float, float]]


class FighterBuilder:
    def __init__(self) -> None:
        self.cells: dict[tuple[str, int, int, int], str] = {}

    def box(
        self,
        part: str,
        palette: str,
        minimum: tuple[int, int, int],
        maximum: tuple[int, int, int],
    ) -> None:
        if part not in PART_PIVOT_CELLS or palette not in PALETTE:
            raise ValueError(f"Unknown fighter part or palette: {part}, {palette}")
        for z in range(minimum[2], maximum[2] + 1):
            for y in range(minimum[1], maximum[1] + 1):
                for x in range(minimum[0], maximum[0] + 1):
                    self.cells[(part, x, y, z)] = palette

    def symmetric_box(
        self,
        palette: str,
        positive_y_minimum: tuple[int, int, int],
        positive_y_maximum: tuple[int, int, int],
    ) -> None:
        self.box("Body", palette, positive_y_minimum, positive_y_maximum)
        self.box(
            "Body",
            palette,
            (positive_y_minimum[0], -positive_y_maximum[1], positive_y_minimum[2]),
            (positive_y_maximum[0], -positive_y_minimum[1], positive_y_maximum[2]),
        )

    def build(self) -> None:
        # Boots and legs: deliberately broad, nearly rectangular masses.
        self.symmetric_box("Night", (-3, 2, -20), (4, 5, -17))
        self.symmetric_box("Hide", (-2, 2, -18), (3, 5, -15))
        self.symmetric_box("Night", (-2, 2, -16), (2, 4, -7))
        self.symmetric_box("Steel", (2, 2, -14), (4, 4, -9))
        self.symmetric_box("Steel", (-2, 2, -8), (3, 5, -6))

        # Pelvis, short tabard, torso, and a single readable breastplate.
        self.box("Body", "Night", (-3, -6, -8), (3, 6, -2))
        self.box("Body", "Hide", (2, -6, -7), (4, 6, -4))
        self.box("Body", "Cloth", (4, -3, -7), (5, 3, -1))
        self.box("Body", "Night", (-4, -7, -2), (4, 7, 8))
        self.box("Body", "Steel", (3, -6, 0), (6, 6, 7))
        self.box("Body", "Ash", (6, -4, 1), (7, 4, 6))
        self.box("Body", "Hide", (-6, -6, -7), (-4, 6, 7))

        # Oversized shoulders and simple arms/hands.
        self.symmetric_box("Steel", (-4, 7, 5), (5, 10, 10))
        self.symmetric_box("Night", (-2, 8, -4), (3, 10, 6))
        self.symmetric_box("Steel", (1, 8, -2), (4, 11, 2))
        self.symmetric_box("Skin", (0, 8, -7), (3, 10, -4))

        # One large head, a readable face plane, and a blocky steel helmet.
        self.box("Body", "Skin", (-3, -4, 9), (4, 4, 16))
        self.box("Body", "Hide", (-5, -5, 10), (-3, 5, 17))
        self.box("Body", "Steel", (-4, -5, 15), (4, 5, 20))
        self.box("Body", "Steel", (-3, -5, 11), (4, -4, 16))
        self.box("Body", "Steel", (-3, 4, 11), (4, 5, 16))
        self.box("Body", "Ash", (4, -5, 15), (5, 5, 17))
        self.box("Body", "Night", (5, -3, 12), (5, -2, 13))
        self.box("Body", "Night", (5, 2, 12), (5, 3, 13))

        # Sword in the existing axe compatibility slot.
        self.box("Axe", "Wood", (-1, -1, -2), (1, 1, 3))
        self.box("Axe", "Ash", (-1, -4, 3), (1, 4, 5))
        self.box("Axe", "Steel", (-1, -1, 5), (1, 1, 20))
        self.box("Axe", "Ash", (0, 0, 20), (0, 0, 23))

        # Thick round-ish shield with a coarse stepped outline.
        radius = 7
        for z in range(-radius, radius + 1):
            for y in range(-radius, radius + 1):
                if max(abs(y), abs(z)) > radius or abs(y) + abs(z) > 11:
                    continue
                boundary = abs(y) + abs(z) >= 9 or max(abs(y), abs(z)) == radius
                palette = "Steel" if boundary else "Hide"
                for x in range(0, 3):
                    self.cells[("Shield", x, y, z)] = palette
        self.box("Shield", "Ash", (3, -2, -2), (4, 2, 2))


def visible_cells(source: dict[tuple[str, int, int, int], str]) -> list[VoxelCell]:
    result: list[VoxelCell] = []
    for (part, x, y, z), palette in source.items():
        if any(
            (part, x + dx, y + dy, z + dz) not in source
            for dx, dy, dz in NEIGHBOURS
        ):
            result.append(VoxelCell(part, palette, x, y, z))
    return sorted(
        result,
        key=lambda cell: (PART_ORDER[cell.part], PALETTE_ORDER[cell.palette], cell.z, cell.y, cell.x),
    )


def stable_uuid(label: str) -> str:
    return str(uuid.uuid5(uuid.NAMESPACE_URL, f"https://emberdeep.local/fighter/{label}"))


def unreal_to_blockbench(point: tuple[float, float, float]) -> list[float]:
    """Map Unreal X-forward/Y-right/Z-up to Blockbench X-right/Y-up/Z-forward."""
    return [point[1], point[2], point[0]]


def blockbench_to_unreal(point: list[float]) -> tuple[float, float, float]:
    return (point[2], point[0], point[1])


def rotate_blockbench(
    point: tuple[float, float, float],
    rotation: tuple[float, float, float],
) -> tuple[float, float, float]:
    """Apply Blockbench/Three.js ZYX Euler rotation to a point."""
    x, y, z = point
    rx, ry, rz = [math.radians(value) for value in rotation]

    y, z = y * math.cos(rx) - z * math.sin(rx), y * math.sin(rx) + z * math.cos(rx)
    x, z = x * math.cos(ry) + z * math.sin(ry), -x * math.sin(ry) + z * math.cos(ry)
    x, y = x * math.cos(rz) - y * math.sin(rz), x * math.sin(rz) + y * math.cos(rz)
    return (x, y, z)


def rotate_unreal(
    point: tuple[float, float, float],
    rotation_bb: tuple[float, float, float],
) -> tuple[float, float, float]:
    rotated_bb = rotate_blockbench(tuple(unreal_to_blockbench(point)), rotation_bb)
    return blockbench_to_unreal(list(rotated_bb))


def matrix_to_quaternion(matrix: tuple[tuple[float, float, float], ...]) -> tuple[float, float, float, float]:
    m00, m01, m02 = matrix[0]
    m10, m11, m12 = matrix[1]
    m20, m21, m22 = matrix[2]
    trace = m00 + m11 + m22
    if trace > 0.0:
        scale = math.sqrt(trace + 1.0) * 2.0
        w = 0.25 * scale
        x = (m21 - m12) / scale
        y = (m02 - m20) / scale
        z = (m10 - m01) / scale
    elif m00 > m11 and m00 > m22:
        scale = math.sqrt(1.0 + m00 - m11 - m22) * 2.0
        w = (m21 - m12) / scale
        x = 0.25 * scale
        y = (m01 + m10) / scale
        z = (m02 + m20) / scale
    elif m11 > m22:
        scale = math.sqrt(1.0 + m11 - m00 - m22) * 2.0
        w = (m02 - m20) / scale
        x = (m01 + m10) / scale
        y = 0.25 * scale
        z = (m12 + m21) / scale
    else:
        scale = math.sqrt(1.0 + m22 - m00 - m11) * 2.0
        w = (m10 - m01) / scale
        x = (m02 + m20) / scale
        y = (m12 + m21) / scale
        z = 0.25 * scale
    length = math.sqrt(x * x + y * y + z * z + w * w)
    return (x / length, y / length, z / length, w / length)


def blockbench_rotation_to_unreal_quaternion(
    rotation_bb: tuple[float, float, float],
) -> tuple[float, float, float, float]:
    columns = [
        rotate_unreal(axis, rotation_bb)
        for axis in ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))
    ]
    matrix = tuple(tuple(columns[column][row] for column in range(3)) for row in range(3))
    return matrix_to_quaternion(matrix)


def greedy_cuboids(source: dict[tuple[str, int, int, int], str]) -> list[AuthoringCuboid]:
    """Merge exact voxel occupancy into editable axis-aligned boxes without overlap."""
    result: list[AuthoringCuboid] = []
    for part in PART_PIVOT_CELLS:
        for palette in PALETTE:
            remaining = {
                (x, y, z)
                for (cell_part, x, y, z), cell_palette in source.items()
                if cell_part == part and cell_palette == palette
            }
            while remaining:
                x0, y0, z0 = min(remaining, key=lambda point: (point[2], point[1], point[0]))
                x1 = x0 + 1
                while (x1, y0, z0) in remaining:
                    x1 += 1

                y1 = y0 + 1
                while all((x, y1, z0) in remaining for x in range(x0, x1)):
                    y1 += 1

                z1 = z0 + 1
                while all(
                    (x, y, z1) in remaining
                    for y in range(y0, y1)
                    for x in range(x0, x1)
                ):
                    z1 += 1

                for z in range(z0, z1):
                    for y in range(y0, y1):
                        for x in range(x0, x1):
                            remaining.remove((x, y, z))
                result.append(AuthoringCuboid(part, palette, (x0, y0, z0), (x1, y1, z1)))
    return result


def palette_texture_data_url() -> str:
    swatch_size = 16
    image = Image.new("RGB", (len(PALETTE) * swatch_size, swatch_size))
    draw = ImageDraw.Draw(image)
    for index, color in enumerate(PALETTE.values()):
        draw.rectangle(
            (index * swatch_size, 0, (index + 1) * swatch_size - 1, swatch_size - 1),
            fill=color,
        )
    output = BytesIO()
    image.save(output, format="PNG", optimize=False)
    return "data:image/png;base64," + base64.b64encode(output.getvalue()).decode("ascii")


def cube_faces(palette: str) -> dict[str, dict[str, object]]:
    swatch_size = 16
    index = PALETTE_ORDER[palette]
    uv = [index * swatch_size, 0, (index + 1) * swatch_size, swatch_size]
    return {
        face: {"uv": uv, "texture": 0}
        for face in ("north", "east", "south", "west", "up", "down")
    }


def write_bbmodel(model: FighterModel) -> bytes:
    cuboids = greedy_cuboids(model.cells)
    group_children: dict[str, list[str]] = {part: [] for part in PART_PIVOT_CELLS}
    elements: list[dict[str, object]] = []
    palette_counts = {palette: 0 for palette in PALETTE}

    for cuboid_index, cuboid in enumerate(cuboids):
        pivot = model.pivots[cuboid.part]
        minimum_world = tuple(cuboid.minimum[index] + pivot[index] for index in range(3))
        maximum_world = tuple(cuboid.maximum[index] + pivot[index] for index in range(3))
        from_bb = unreal_to_blockbench(minimum_world)
        to_bb = unreal_to_blockbench(maximum_world)
        element_uuid = stable_uuid(
            f"cube/{cuboid.part}/{cuboid.palette}/{cuboid.minimum}/{cuboid.maximum}/{cuboid_index}"
        )
        palette_number = palette_counts[cuboid.palette]
        palette_counts[cuboid.palette] += 1
        group_children[cuboid.part].append(element_uuid)
        elements.append(
            {
                "name": f"{cuboid.palette}__{palette_number:03d}",
                "box_uv": False,
                "render_order": "default",
                "locked": False,
                "allow_mirror_modeling": True,
                "from": from_bb,
                "to": to_bb,
                "autouv": 0,
                "color": PALETTE_ORDER[cuboid.palette] % 8,
                "origin": [(from_bb[index] + to_bb[index]) / 2 for index in range(3)],
                "faces": cube_faces(cuboid.palette),
                "type": "cube",
                "uuid": element_uuid,
            }
        )

    groups: list[dict[str, object]] = []
    outliner: list[dict[str, object]] = []
    for part in PART_PIVOT_CELLS:
        group_uuid = stable_uuid(f"group/{part}")
        group: dict[str, object] = {
            "name": PART_DISPLAY_NAMES[part],
            "origin": unreal_to_blockbench(model.pivots[part]),
            "color": PART_ORDER[part],
            "uuid": group_uuid,
        }
        if any(abs(value) > 0.0001 for value in model.rotations[part]):
            group["rotation"] = list(model.rotations[part])
        groups.append(group)
        outliner.append(
            {
                **group,
                "export": True,
                "isOpen": True,
                "locked": False,
                "visibility": True,
                "autouv": 0,
                "children": group_children[part],
            }
        )

    texture_uuid = stable_uuid("texture/palette")
    document = {
        "meta": {"format_version": "5.0", "model_format": "free", "box_uv": False},
        "name": "Emberdeep Low-Density Fighter v0",
        "model_identifier": "emberdeep_fighter_v0",
        "visible_box": [1, 1, 0],
        "variable_placeholders": "",
        "variable_placeholder_buttons": [],
        "timeline_setups": [],
        "unhandled_root_fields": {},
        "resolution": {"width": len(PALETTE) * 16, "height": 16},
        "elements": elements,
        "groups": groups,
        "outliner": outliner,
        "textures": [
            {
                "name": "fighter_palette.png",
                "folder": "",
                "namespace": "",
                "id": "0",
                "particle": False,
                "render_mode": "default",
                "render_sides": "auto",
                "frame_time": 1,
                "frame_order_type": "loop",
                "frame_order": "",
                "frame_interpolate": False,
                "visible": True,
                "internal": True,
                "saved": True,
                "uuid": texture_uuid,
                "source": palette_texture_data_url(),
            }
        ],
    }
    return (json.dumps(document, indent=2) + "\n").encode("utf-8")


def require_grid_value(value: object, description: str) -> int:
    if not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"{description} must be a finite number")
    rounded = round(value)
    if abs(value - rounded) > 0.0001:
        raise ValueError(
            f"{description} is {value}; Emberdeep solid geometry must stay on whole Blockbench grid units"
        )
    return int(rounded)


def require_number(value: object, description: str) -> float:
    if not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"{description} must be a finite number")
    return float(value)


def read_bbmodel(path: Path) -> FighterModel:
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("meta", {}).get("model_format") != "free":
        raise ValueError("fighter_v0.bbmodel must remain a Blockbench Generic Model")

    elements = {element.get("uuid"): element for element in document.get("elements", [])}
    if None in elements:
        raise ValueError("Every Blockbench cube must have a UUID")

    group_templates = {
        group.get("uuid"): group
        for group in document.get("groups", [])
        if isinstance(group, dict) and group.get("uuid")
    }
    def merge_group(node: object) -> object:
        if not isinstance(node, dict):
            return node
        merged = {**group_templates.get(node.get("uuid"), {}), **node}
        children = merged.get("children", [])
        if isinstance(children, list):
            merged["children"] = [merge_group(child) for child in children]
        return merged

    top_groups = []
    for outliner_group in document.get("outliner", []):
        if not isinstance(outliner_group, dict):
            continue
        top_groups.append(merge_group(outliner_group))
    def nested_groups(nodes: list[object]):
        for node in nodes:
            if not isinstance(node, dict):
                continue
            yield node
            children = node.get("children", [])
            if isinstance(children, list):
                yield from nested_groups(children)

    groups_by_name: dict[str, dict[str, object]] = {}
    for group in nested_groups(top_groups):
        for internal_name, aliases in PART_DISPLAY_ALIASES.items():
            if group.get("name") in aliases:
                if internal_name in groups_by_name:
                    raise ValueError(f"Blockbench contains more than one {PART_DISPLAY_NAMES[internal_name]} group")
                groups_by_name[internal_name] = group
    if set(groups_by_name) != set(PART_PIVOT_CELLS):
        raise ValueError("Blockbench outliner must contain Body, Sword, and Shield groups")

    pivots: dict[str, tuple[int, int, int]] = {}
    rotations: dict[str, tuple[float, float, float]] = {}
    cells: dict[tuple[str, int, int, int], str] = {}

    for part in PART_PIVOT_CELLS:
        group = groups_by_name[part]
        origin = group.get("origin", [0, 0, 0])
        if not isinstance(origin, list) or len(origin) != 3:
            raise ValueError(f"{part} group origin must contain three coordinates")
        origin_bb = [require_grid_value(value, f"{part} group origin") for value in origin]
        pivot_ue = blockbench_to_unreal(origin_bb)
        pivots[part] = tuple(int(value) for value in pivot_ue)

        rotation = group.get("rotation", [0, 0, 0])
        if not isinstance(rotation, list) or len(rotation) != 3:
            raise ValueError(f"{part} group rotation must contain three angles")
        rotation_values = tuple(require_number(value, f"{part} group rotation") for value in rotation)
        if part == "Body" and any(abs(value) > 0.0001 for value in rotation_values):
            raise ValueError("Rotate the character actor in Unreal; the Blockbench Body group must stay unrotated")
        rotations[part] = rotation_values

        child_ids = group.get("children", [])
        if not isinstance(child_ids, list):
            raise ValueError(f"{part} group children are invalid")

        def cube_ids(children: list[object]):
            for child in children:
                if isinstance(child, str):
                    yield child
                elif isinstance(child, dict):
                    child_name = child.get("name")
                    if any(
                        child_name in aliases
                        for other_part, aliases in PART_DISPLAY_ALIASES.items()
                        if other_part != part
                    ):
                        # Nested equipment belongs to its own rigid part, not Body.
                        continue
                    nested = child.get("children", [])
                    if not isinstance(nested, list):
                        raise ValueError(f"Nested group in {part} has invalid children")
                    yield from cube_ids(nested)
                else:
                    raise ValueError(f"{part} contains an unsupported outliner entry")

        for child_id in cube_ids(child_ids):
            if not isinstance(child_id, str) or child_id not in elements:
                raise ValueError(f"{part} contains a missing cube: {child_id}")
            element = elements[child_id]
            if element.get("type", "cube") != "cube":
                raise ValueError(f"{element.get('name', child_id)} is not a cube")
            if element.get("rotation", [0, 0, 0]) != [0, 0, 0]:
                raise ValueError(
                    f"{element.get('name', child_id)} has an individual rotation; rotate the Shield group instead"
                )
            name = str(element.get("name", ""))
            palette = name.split("__", 1)[0]
            if palette not in PALETTE:
                raise ValueError(
                    f"Cube '{name}' has no Emberdeep palette prefix; duplicate and retain names such as Steel__001"
                )

            from_bb = element.get("from")
            to_bb = element.get("to")
            if not isinstance(from_bb, list) or not isinstance(to_bb, list) or len(from_bb) != 3 or len(to_bb) != 3:
                raise ValueError(f"Cube '{name}' must have three from/to coordinates")
            from_grid = [require_grid_value(value, f"{name} from") for value in from_bb]
            to_grid = [require_grid_value(value, f"{name} to") for value in to_bb]
            if any(to_grid[index] <= from_grid[index] for index in range(3)):
                raise ValueError(f"Cube '{name}' must have positive size on every axis")

            minimum_world = blockbench_to_unreal(from_grid)
            maximum_world = blockbench_to_unreal(to_grid)
            minimum = tuple(int(minimum_world[index] - pivots[part][index]) for index in range(3))
            maximum = tuple(int(maximum_world[index] - pivots[part][index]) for index in range(3))
            for z in range(minimum[2], maximum[2]):
                for y in range(minimum[1], maximum[1]):
                    for x in range(minimum[0], maximum[0]):
                        key = (part, x, y, z)
                        previous = cells.get(key)
                        if previous is not None and previous != palette:
                            raise ValueError(
                                f"Cube '{name}' overlaps {previous} at local voxel {(x, y, z)} in {part}"
                            )
                        cells[key] = palette

    return FighterModel(cells=cells, pivots=pivots, rotations=rotations)


def hash_cell(cell: VoxelCell) -> int:
    value = 2166136261
    seed = PALETTE_ORDER[cell.palette] + PART_ORDER[cell.part] * 17
    for component in (cell.x // 2, cell.y // 2, cell.z // 2, seed):
        value = ((value ^ (component & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
    return value


def material_color(cell: VoxelCell) -> str:
    selector = hash_cell(cell) % 16
    shade_index = 0 if selector < 3 else (2 if selector == 15 else 1)
    return MATERIAL_SHADES[cell.palette][shade_index]


def write_cpp(model: FighterModel, cells: list[VoxelCell]) -> bytes:
    runtime_cells = [cell for cell in cells if cell.part == "Body"]
    axe_pivot = tuple(value * VOXEL_UNIT_CM for value in model.pivots["Axe"])
    shield_pivot = tuple(value * VOXEL_UNIT_CM for value in model.pivots["Shield"])
    axe_quaternion = blockbench_rotation_to_unreal_quaternion(model.rotations["Axe"])
    shield_quaternion = blockbench_rotation_to_unreal_quaternion(model.rotations["Shield"])
    lines = [
        "// Generated by SourceAssets/Characters/Fighter/generate_fighter.py. Do not edit by hand.",
        f"static constexpr float GThorgrimVoxelUnitCm = {VOXEL_UNIT_CM:.1f}f;",
        "// Compatibility symbols let the prototype reuse its current rigid equipment/animation path.",
        'static constexpr const TCHAR* GPlayableVoxelCharacterName = TEXT("LowDensityFighter");',
        "static const FVector GThorgrimAxePivot(%.1ff, %.1ff, %.1ff);" % axe_pivot,
        "static const FVector GThorgrimShieldPivot(%.1ff, %.1ff, %.1ff);" % shield_pivot,
        "static const FQuat GPlayableAxeRestingRotation(%.9ff, %.9ff, %.9ff, %.9ff);" % axe_quaternion,
        "static const FQuat GPlayableShieldRestingRotation(%.9ff, %.9ff, %.9ff, %.9ff);"
        % shield_quaternion,
        "",
        "static const FThorgrimVoxelCell GThorgrimVoxelCells[] =",
        "{",
    ]
    for cell in runtime_cells:
        lines.append(
            "\t{ EThorgrimPart::%s, EThorgrimPalette::%s, %d, %d, %d },"
            % (cell.part, cell.palette, cell.x, cell.y, cell.z)
        )
    lines.extend(("};", f"static constexpr int32 GThorgrimVoxelCount = {len(runtime_cells)};", ""))
    return "\n".join(lines).encode("utf-8")


def preview_bytes(model: FighterModel, cells: list[VoxelCell]) -> bytes:
    logical_width, logical_height = 560, 560
    image = Image.new("RGB", (logical_width, logical_height), "#090C10")
    draw = ImageDraw.Draw(image)

    def world(cell: VoxelCell) -> tuple[float, float, float]:
        pivot = model.pivots[cell.part]
        local_x = (cell.x + 0.5) * VOXEL_UNIT_CM
        local_y = (cell.y + 0.5) * VOXEL_UNIT_CM
        local_x, local_y, local_z = rotate_unreal(
            (local_x, local_y, (cell.z + 0.5) * VOXEL_UNIT_CM),
            model.rotations[cell.part],
        )
        return (
            pivot[0] * VOXEL_UNIT_CM + local_x,
            pivot[1] * VOXEL_UNIT_CM + local_y,
            pivot[2] * VOXEL_UNIT_CM + local_z,
        )

    def project(point: tuple[float, float, float]) -> tuple[float, float]:
        x, y, z = point
        return (logical_width / 2 + (x + y) * 1.35, 390 + (-x + y) * 0.72 - z * 1.62)

    half = VOXEL_UNIT_CM / 2
    ordered = sorted(cells, key=lambda cell: (-world(cell)[0] + world(cell)[1] + world(cell)[2] * 1.8))
    for cell in ordered:
        cx, cy, cz = world(cell)
        vertices = [
            (cx + x, cy + y, cz + z)
            for x, y, z in (
                (-half, -half, -half), (half, -half, -half), (half, half, -half), (-half, half, -half),
                (-half, -half, half), (half, -half, half), (half, half, half), (-half, half, half),
            )
        ]
        points = [project(vertex) for vertex in vertices]
        base = material_color(cell).lstrip("#")
        rgb = tuple(int(base[index:index + 2], 16) for index in (0, 2, 4))
        shade = lambda factor: tuple(max(0, min(255, round(channel * factor))) for channel in rgb)
        draw.polygon([points[index] for index in (0, 4, 7, 3)], fill=shade(0.60))
        draw.polygon([points[index] for index in (3, 2, 6, 7)], fill=shade(0.78))
        draw.polygon([points[index] for index in (4, 5, 6, 7)], fill=shade(1.10))

    draw.rectangle((12, 12, logical_width - 13, logical_height - 13), outline="#6D4B2C", width=2)
    draw.text((26, 24), "LOW-DENSITY FIGHTER  /  PLAYABLE V0", fill="#D2C2A2")
    draw.text(
        (26, logical_height - 42),
        f"{len(cells)} visible 4 cm cells  |  forms authored in 2x2 modules",
        fill="#8C8273",
    )
    output = BytesIO()
    image.resize((1120, 1120), Image.Resampling.NEAREST).save(output, format="PNG", optimize=False)
    return output.getvalue()


def bootstrap_model(force: bool) -> FighterModel:
    builder = FighterBuilder()
    builder.build()
    model = FighterModel(
        cells=dict(builder.cells),
        pivots=dict(PART_PIVOT_CELLS),
        rotations=dict(PART_ROTATION_DEGREES),
    )
    if BLOCKBENCH_OUTPUT.exists() and not force:
        raise SystemExit(
            f"Refusing to overwrite editable Blockbench master: {BLOCKBENCH_OUTPUT}\n"
            "Use --force-bootstrap-blockbench only when you intentionally want to reset all manual edits."
        )
    BLOCKBENCH_OUTPUT.write_bytes(write_bbmodel(model))
    return model


def generate(check: bool, bootstrap: bool, force_bootstrap: bool) -> list[VoxelCell]:
    if bootstrap or force_bootstrap:
        model = bootstrap_model(force_bootstrap)
    else:
        if not BLOCKBENCH_OUTPUT.exists():
            raise SystemExit(
                f"Missing editable Blockbench master: {BLOCKBENCH_OUTPUT}\n"
                "Run with --bootstrap-blockbench once to create it."
            )
        model = read_bbmodel(BLOCKBENCH_OUTPUT)

    cells = visible_cells(model.cells)
    if not 1_500 <= len(cells) <= 4_000:
        raise ValueError(f"Unexpected low-density fighter voxel count: {len(cells)}")
    if {cell.part for cell in cells} != set(PART_PIVOT_CELLS):
        raise ValueError("Fighter lost a rigid part")

    cpp = write_cpp(model, cells)
    preview = preview_bytes(model, cells)
    if check:
        for path, content in ((CPP_OUTPUT, cpp), (PREVIEW_OUTPUT, preview)):
            if not path.exists() or path.read_bytes() != content:
                raise SystemExit(f"Generated output is stale: {path}")
    else:
        for path, content in ((CPP_OUTPUT, cpp), (PREVIEW_OUTPUT, preview)):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
    return cells


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--bootstrap-blockbench", action="store_true")
    parser.add_argument("--force-bootstrap-blockbench", action="store_true")
    arguments = parser.parse_args()
    if arguments.check and (arguments.bootstrap_blockbench or arguments.force_bootstrap_blockbench):
        parser.error("--check cannot be combined with a Blockbench bootstrap option")
    cells = generate(
        arguments.check,
        arguments.bootstrap_blockbench,
        arguments.force_bootstrap_blockbench,
    )
    print(
        f"Generated low-density fighter with {len(cells)} visible {VOXEL_UNIT_CM:.1f} cm cells "
        f"using {DESIGN_MODULE_CELLS}x{DESIGN_MODULE_CELLS} design modules"
    )


if __name__ == "__main__":
    main()
