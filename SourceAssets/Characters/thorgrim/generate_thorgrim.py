"""Generate Thorgrim's v0 voxel model data, preview, and interchange mesh.

The Python block description is the editable source of truth. Running this file
updates the C++ runtime data used by Emberdeep, a lightweight OBJ export, and a
nearest-neighbour preview without requiring Blender.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import math

import numpy as np
from PIL import Image, ImageDraw, ImageFont


REPO_ROOT = Path(__file__).resolve().parents[3]
ASSET_DIR = Path(__file__).resolve().parent
EXPORT_DIR = ASSET_DIR / "exports"
CPP_OUTPUT = REPO_ROOT / "Source" / "Emberdeep" / "Private" / "Gameplay" / "ThorgrimVoxelData.inl"
PREVIEW_OUTPUT = ASSET_DIR / "thorgrim_v0_preview.png"
OBJ_OUTPUT = EXPORT_DIR / "thorgrim_v0.obj"
MTL_OUTPUT = EXPORT_DIR / "thorgrim_v0.mtl"

# The runtime character origin is at capsule centre; authoring origin is at the feet.
RUNTIME_Z_OFFSET_CM = -80.0
MODEL_SCALE = 165.0 / 175.0
VOXEL_UNIT_CM = 4.0

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

PART_PIVOTS = {
    "Body": (0.0, 0.0, 0.0),
    "Axe": (12.0, 66.0, 51.0),
    "Shield": (38.0, -69.0, 77.0),
}

# Equipment pivots are snapped to the same actor-local grid as the body. They
# may rotate during animation, but their authored bind-pose voxels remain on
# the one shared four-centimetre lattice.
RUNTIME_PART_PIVOTS = {
    "Body": (0.0, 0.0, 0.0),
    "Axe": (12.0, 64.0, -32.0),
    "Shield": (36.0, -64.0, -8.0),
}


@dataclass(frozen=True)
class Block:
    name: str
    palette: str
    center: tuple[float, float, float]
    size: tuple[float, float, float]
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)  # pitch, yaw, roll


@dataclass(frozen=True)
class VoxelCell:
    part: str
    palette: str
    x: int
    y: int
    z: int


BLOCKS: list[Block] = []


def box(
    name: str,
    palette: str,
    center: tuple[float, float, float],
    size: tuple[float, float, float],
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> None:
    if palette not in PALETTE:
        raise ValueError(f"Unknown palette {palette!r}")
    if min(size) <= 0:
        raise ValueError(f"Invalid size for {name}: {size}")
    BLOCKS.append(Block(name, palette, center, size, rotation))


def symmetric_box(
    name: str,
    palette: str,
    center_positive_y: tuple[float, float, float],
    size: tuple[float, float, float],
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> None:
    x, y, z = center_positive_y
    pitch, yaw, roll = rotation
    box(f"{name}_r", palette, (x, y, z), size, rotation)
    box(f"{name}_l", palette, (x, -y, z), size, (pitch, -yaw, -roll))


def build_model() -> None:
    # Boots, legs, and a wide grounded stance.
    symmetric_box("boot_sole", "Night", (6, 24, 7), (36, 26, 10))
    symmetric_box("boot", "Hide", (2, 24, 15), (32, 24, 18))
    symmetric_box("boot_toe_cap", "Steel", (18, 24, 16), (8, 24, 14))
    symmetric_box("boot_toe_ridge", "Ash", (22, 24, 18), (4, 12, 8))
    symmetric_box("boot_ankle_band", "Steel", (1, 24, 23), (26, 26, 6))
    symmetric_box("lower_leg", "Fur", (0, 22, 34), (24, 24, 30))
    symmetric_box("shin_plate", "Steel", (13, 22, 35), (6, 18, 22))
    symmetric_box("shin_ridge", "Ash", (17, 22, 35), (4, 8, 18))
    symmetric_box("knee", "Steel", (10, 22, 51), (16, 24, 12))
    symmetric_box("thigh", "Night", (0, 21, 58), (28, 26, 28))

    # Underlayer, skirt armour, and belt.
    box("pelvis", "Night", (0, 0, 63), (38, 58, 30))
    box("waist_hide", "Hide", (2, 0, 68), (42, 70, 16))
    box("belt", "Steel", (20, 0, 68), (8, 72, 8))
    symmetric_box("front_tasset", "Cloth", (22, 16, 50), (8, 24, 32), (0, 0, 4))
    symmetric_box("side_tasset", "Hide", (5, 36, 52), (24, 12, 30), (0, 0, 8))
    symmetric_box("tasset_plate", "Steel", (26, 16, 51), (5, 20, 12))
    symmetric_box("belt_pouch", "Hide", (4, 38, 66), (20, 14, 18))
    symmetric_box("belt_pouch_flap", "Fur", (15, 38, 70), (5, 12, 8))
    symmetric_box("belt_ring", "Ash", (24, 31, 67), (4, 8, 8), (0, 0, 45))

    # Barrel torso with layered undercloth, hide, and front fur.
    box("torso_core", "Night", (0, 0, 92), (48, 76, 58))
    box("torso_hide", "Hide", (11, 0, 92), (32, 82, 50))
    box("chest_fur_mass", "Fur", (27, 0, 99), (18, 66, 42))
    for y in (-26, -10, 10, 26):
        box(f"chest_fur_lock_{y:+d}", "Fur", (39, y, 91 + (abs(y) % 12)), (12, 12, 28), (0, 0, -6 if y > 0 else 6))
    symmetric_box("chest_strap", "Hide", (34, 22, 103), (6, 12, 42), (0, 0, 24))
    box("sternum_plate", "Steel", (43, 0, 101), (6, 16, 24))
    for index, (y, z) in enumerate(((-27, 81), (-14, 78), (14, 78), (27, 81), (-29, 99), (29, 99))):
        box(f"chest_scale_{index}", "Steel" if index % 2 == 0 else "Ash", (43, y, z), (7, 13, 10), (0, 0, -4 if y > 0 else 4))
    for index, y in enumerate((-26, -13, 0, 13, 26)):
        box(f"waist_lamellar_{index}", "Hide" if index % 2 else "Steel", (26, y, 72), (8, 12, 12))

    # Fur mantle: broad clusters are more important than surface noise.
    box("mantle_back", "Fur", (-22, 0, 112), (28, 96, 38))
    symmetric_box("mantle_shoulder_outer", "Fur", (-2, 52, 112), (42, 34, 34), (0, 0, 8))
    symmetric_box("mantle_shoulder_top", "Fur", (-4, 38, 127), (36, 32, 20), (0, 0, 4))
    symmetric_box("mantle_shoulder_front", "Fur", (23, 44, 112), (22, 26, 30), (0, 0, -8))
    for side in (-1, 1):
        for row, z in enumerate((100, 112, 124)):
            for col, y in enumerate((34, 48, 60)):
                if row == 2 and col == 2:
                    continue
                box(
                    f"fur_cluster_{side:+d}_{row}_{col}",
                    "Fur",
                    (-10 + row * 3, side * y, z),
                    (18, 16, 14),
                    (0, 0, side * (5 - row * 2)),
                )
        for index, (x, y, z, size) in enumerate(
            (
                (17, 63, 124, (13, 12, 14)),
                (23, 66, 112, (12, 11, 15)),
                (21, 65, 98, (11, 10, 15)),
                (4, 62, 132, (14, 12, 11)),
                (-13, 58, 136, (14, 12, 10)),
            )
        ):
            box(f"mantle_edge_{side:+d}_{index}", "Fur", (x, side * y, z), size, (0, 0, side * (8 + index * 2)))
    for index, y in enumerate((-30, -18, -6, 6, 18, 30)):
        box(f"fur_collar_{index}", "Fur", (18 - abs(y) * 0.15, y, 132), (16, 12, 16), (0, 0, -y * 0.2))

    # Small steel plates break up the fur without turning it into surface noise.
    symmetric_box("shoulder_plate_outer", "Steel", (13, 62, 111), (16, 12, 28), (0, 0, 8))
    symmetric_box("shoulder_plate_top", "Ash", (5, 51, 130), (20, 18, 8), (0, 0, 5))
    symmetric_box("shoulder_plate_front", "Steel", (30, 52, 113), (8, 20, 18), (0, 0, -8))

    # Arms, oversized bracers, and hands.
    symmetric_box("upper_arm", "Hide", (2, 55, 90), (30, 28, 38), (0, 0, 5))
    symmetric_box("upper_arm_plate", "Steel", (13, 58, 93), (12, 30, 24), (0, 0, 7))
    symmetric_box("forearm", "Skin", (8, 58, 61), (22, 22, 30), (0, 0, -5))
    symmetric_box("bracer", "Steel", (13, 59, 68), (16, 26, 24), (0, 0, -5))
    symmetric_box("bracer_hide", "Hide", (22, 59, 68), (5, 20, 18), (0, 0, -5))
    symmetric_box("hand", "Skin", (12, 59, 43), (20, 20, 18))
    symmetric_box("bracer_band_top", "Ash", (16, 59, 76), (6, 27, 5), (0, 0, -5))
    symmetric_box("bracer_band_bottom", "Ash", (18, 59, 60), (6, 24, 5), (0, 0, -5))
    symmetric_box("knuckle", "Hide", (23, 59, 45), (5, 14, 8))

    # Head, swept hair, brows, nose, and a segmented beard.
    box("head", "Skin", (5, 0, 142), (36, 36, 34))
    box("nose", "Skin", (26, 0, 141), (8, 8, 10))
    symmetric_box("ear", "Skin", (4, 20, 141), (10, 6, 12))
    box("hair_back", "Fur", (-13, 0, 145), (16, 38, 34))
    box("hair_top", "Fur", (3, 0, 163), (34, 38, 10))
    box("hair_swept_1", "Fur", (7, -8, 170), (30, 24, 8), (0, -8, 0))
    box("hair_swept_2", "Fur", (-5, 10, 171), (24, 20, 8), (0, 12, 0))
    symmetric_box("hair_temple", "Fur", (14, 17, 154), (14, 8, 18), (0, 0, 8))
    symmetric_box("hair_side_lock", "Fur", (-4, 19, 135), (10, 8, 20), (0, 0, 5))
    symmetric_box("brow", "Fur", (25, 9, 149), (5, 12, 5), (0, 0, 8))
    symmetric_box("eye", "Night", (29, 9, 145), (3, 5, 3))
    symmetric_box("moustache", "Fur", (30, 9, 134), (12, 15, 8), (0, 0, 12))
    box("beard_center_top", "Fur", (27, 0, 127), (18, 18, 22))
    box("beard_center_mid", "Fur", (25, 0, 112), (16, 16, 20))
    box("beard_center_tip", "Fur", (21, 0, 99), (12, 12, 16))
    symmetric_box("beard_outer_top", "Fur", (25, 15, 128), (16, 14, 24), (0, 0, 8))
    symmetric_box("beard_outer_mid", "Fur", (23, 17, 112), (14, 12, 20), (0, 0, 12))
    symmetric_box("beard_outer_tip", "Fur", (18, 16, 99), (10, 10, 14), (0, 0, 15))
    for index, (y, z, length) in enumerate(((-11, 121, 16), (-6, 115, 18), (6, 115, 18), (11, 121, 16), (-9, 103, 15), (9, 103, 15))):
        box(f"beard_lock_{index}", "Fur", (34 - abs(y) * 0.25, y, z), (8, 8, length), (0, 0, -y * 0.7))
    symmetric_box("cheek_plane", "Skin", (26, 14, 140), (6, 8, 14))
    box("mouth_shadow", "Night", (31, 0, 132), (3, 9, 3))

    # Mammoth tusks curve down and inward as visibly stepped voxel chains.
    tusk_steps = (
        ((30, 51, 121), (12, 14, 22), (8, 0, 18)),
        ((37, 49, 106), (11, 13, 22), (15, 0, 24)),
        ((43, 44, 91), (10, 12, 20), (20, 0, 28)),
        ((47, 36, 78), (9, 10, 18), (24, 0, 34)),
        ((48, 28, 68), (7, 8, 14), (28, 0, 38)),
    )
    for index, (center, size, rotation) in enumerate(tusk_steps):
        symmetric_box(f"tusk_{index}", "Bone", center, size, rotation)

    # Trophy skull at the belt and small bone tokens.
    box("belt_skull_face", "Bone", (29, 0, 67), (8, 18, 14))
    symmetric_box("belt_skull_eye", "Night", (34, 5, 70), (3, 4, 4))
    symmetric_box("belt_skull_horn", "Bone", (27, 12, 72), (6, 8, 5), (0, 0, 20))
    box("belt_skull_jaw", "Bone", (31, 0, 59), (6, 10, 6))
    symmetric_box("trophy_token", "Bone", (20, 36, 55), (8, 8, 14), (0, 0, 12))
    symmetric_box("trophy_rope", "Hide", (12, 33, 56), (5, 5, 22), (0, 0, 12))
    symmetric_box("trophy_bead", "Ash", (18, 34, 46), (7, 7, 7))

    # Bone-and-steel axe in Thorgrim's right hand (+Y).
    box("axe_haft", "Wood", (12, 66, 91), (8, 8, 108), (0, 0, -3))
    box("axe_grip", "Hide", (14, 66, 51), (12, 12, 26), (0, 0, -3))
    box("axe_head_core", "Steel", (13, 66, 145), (28, 16, 20))
    box("axe_blade_upper", "Ash", (30, 66, 151), (24, 14, 14), (0, 0, 12))
    box("axe_blade_mid", "Steel", (38, 66, 141), (28, 14, 16), (0, 0, 20))
    box("axe_blade_lower", "Steel", (31, 66, 130), (22, 14, 12), (0, 0, 30))
    box("axe_back_spike", "Bone", (-8, 66, 145), (18, 12, 10), (0, 0, -18))
    box("axe_edge_upper", "Ash", (42, 66, 151), (9, 15, 6), (0, 0, 18))
    box("axe_edge_lower", "Ash", (43, 66, 136), (9, 15, 7), (0, 0, 28))
    box("axe_head_bind", "Hide", (8, 66, 140), (12, 18, 12))

    # Round tusk shield on the left arm (-Y), built as stepped wood and rim plates.
    shield_x, shield_y, shield_z = 38, -69, 77
    for index, y_offset in enumerate((-24, -8, 8, 24)):
        height = 56 if abs(y_offset) < 16 else 44
        box(
            f"shield_plank_{index}",
            "Wood" if index % 2 == 0 else "Hide",
            (shield_x, shield_y + y_offset, shield_z),
            (10, 17, height),
        )
    box("shield_rim_top", "Steel", (shield_x + 2, shield_y, shield_z + 35), (14, 50, 10))
    box("shield_rim_bottom", "Steel", (shield_x + 2, shield_y, shield_z - 35), (14, 50, 10))
    box("shield_rim_outer", "Steel", (shield_x + 2, shield_y - 34, shield_z), (14, 10, 50))
    box("shield_rim_inner", "Steel", (shield_x + 2, shield_y + 34, shield_z), (14, 10, 50))
    for index, (dy, dz, roll) in enumerate(((-26, 27, -45), (26, 27, 45), (-26, -27, 45), (26, -27, -45))):
        box(
            f"shield_rim_corner_{index}",
            "Ash",
            (shield_x + 3, shield_y + dy, shield_z + dz),
            (15, 10, 30),
            (0, 0, roll),
        )
    box("shield_boss", "Steel", (shield_x + 9, shield_y, shield_z), (14, 22, 22), (0, 45, 0))
    box("shield_boss_face", "Ash", (shield_x + 17, shield_y, shield_z), (5, 10, 10), (0, 45, 0))
    for index, (dy, dz) in enumerate(((0, 30), (0, -30), (-30, 0), (30, 0), (-22, 22), (22, 22), (-22, -22), (22, -22))):
        box(f"shield_rivet_{index}", "Ash", (shield_x + 10, shield_y + dy, shield_z + dz), (5, 6, 6))
    symmetric_shield_tusks = ((-21, 16, -18), (21, 16, 18), (-21, -16, 18), (21, -16, -18))
    for index, (dy, dz, roll) in enumerate(symmetric_shield_tusks):
        box(
            f"shield_tusk_{index}",
            "Bone",
            (shield_x + 12, shield_y + dy, shield_z + dz),
            (10, 8, 24),
            (0, 0, roll),
        )


def rotation_matrix(rotation: tuple[float, float, float]) -> np.ndarray:
    pitch, yaw, roll = (math.radians(value) for value in rotation)
    rx = np.array(((1, 0, 0), (0, math.cos(roll), -math.sin(roll)), (0, math.sin(roll), math.cos(roll))))
    ry = np.array(((math.cos(pitch), 0, math.sin(pitch)), (0, 1, 0), (-math.sin(pitch), 0, math.cos(pitch))))
    rz = np.array(((math.cos(yaw), -math.sin(yaw), 0), (math.sin(yaw), math.cos(yaw), 0), (0, 0, 1)))
    return rz @ ry @ rx


def block_vertices(block: Block) -> np.ndarray:
    sx, sy, sz = (value / 2.0 for value in block.size)
    local = np.array(
        (
            (-sx, -sy, -sz),
            (sx, -sy, -sz),
            (sx, sy, -sz),
            (-sx, sy, -sz),
            (-sx, -sy, sz),
            (sx, -sy, sz),
            (sx, sy, sz),
            (-sx, sy, sz),
        ),
        dtype=float,
    )
    return local @ rotation_matrix(block.rotation).T + np.asarray(block.center)


def block_part(block: Block) -> str:
    if block.name.startswith("axe_"):
        return "Axe"
    if block.name.startswith("shield_"):
        return "Shield"
    return "Body"


NEIGHBOURS = (
    (1, 0, 0),
    (-1, 0, 0),
    (0, 1, 0),
    (0, -1, 0),
    (0, 0, 1),
    (0, 0, -1),
)


def voxel_center(voxel: VoxelCell) -> np.ndarray:
    return np.asarray(
        (
            (voxel.x + 0.5) * VOXEL_UNIT_CM,
            (voxel.y + 0.5) * VOXEL_UNIT_CM,
            (voxel.z + 0.5) * VOXEL_UNIT_CM,
        )
    )


def voxel_vertices(voxel: VoxelCell) -> np.ndarray:
    half = VOXEL_UNIT_CM / 2.0
    center = voxel_center(voxel)
    return np.asarray(
        (
            (-half, -half, -half),
            (half, -half, -half),
            (half, half, -half),
            (-half, half, -half),
            (-half, -half, half),
            (half, -half, half),
            (half, half, half),
            (-half, half, half),
        )
    ) + center


def voxelize_model() -> list[VoxelCell]:
    """Rasterize every authored cluster onto one actor-local voxel lattice."""

    occupied: dict[tuple[str, int, int, int], str] = {}
    for block in BLOCKS:
        part = block_part(block)
        center = np.asarray(block.center) * MODEL_SCALE
        center[2] += RUNTIME_Z_OFFSET_CM
        size = np.asarray(block.size) * MODEL_SCALE
        rotation = rotation_matrix(block.rotation)
        vertices = block_vertices(block) * MODEL_SCALE
        vertices[:, 2] += RUNTIME_Z_OFFSET_CM
        minimum = vertices.min(axis=0)
        maximum = vertices.max(axis=0)
        wrote_voxel = False

        ranges = tuple(
            range(
                math.floor(minimum[axis] / VOXEL_UNIT_CM) - 1,
                math.floor(maximum[axis] / VOXEL_UNIT_CM) + 2,
            )
            for axis in range(3)
        )
        for x in ranges[0]:
            for y in ranges[1]:
                for z in ranges[2]:
                    sample = np.asarray(
                        (
                            (x + 0.5) * VOXEL_UNIT_CM,
                            (y + 0.5) * VOXEL_UNIT_CM,
                            (z + 0.5) * VOXEL_UNIT_CM,
                        )
                    )
                    local = (sample - center) @ rotation
                    if np.all(np.abs(local) <= size / 2.0 + 1e-5):
                        occupied[(part, x, y, z)] = block.palette
                        wrote_voxel = True

        # Preserve sub-grid accents such as eyes and rivets by assigning their
        # nearest cell. Later detail blocks intentionally override base masses.
        if not wrote_voxel:
            x, y, z = (math.floor(value / VOXEL_UNIT_CM) for value in center)
            occupied[(part, x, y, z)] = block.palette

    visible: list[VoxelCell] = []
    for (part, x, y, z), palette in occupied.items():
        if any(
            occupied.get((part, x + dx, y + dy, z + dz)) != palette
            for dx, dy, dz in NEIGHBOURS
        ):
            visible.append(VoxelCell(part, palette, x, y, z))

    part_order = {name: index for index, name in enumerate(PART_PIVOTS)}
    palette_order = {name: index for index, name in enumerate(PALETTE)}
    return sorted(
        visible,
        key=lambda voxel: (
            part_order[voxel.part],
            palette_order[voxel.palette],
            voxel.z,
            voxel.y,
            voxel.x,
        ),
    )


def validate_model(voxels: list[VoxelCell]) -> None:
    names = [block.name for block in BLOCKS]
    if len(names) != len(set(names)):
        raise ValueError("Every Thorgrim block must have a unique name")
    if len(BLOCKS) < 100:
        raise ValueError("Thorgrim has fallen below the approved voxel-density floor")
    used_palettes = {block.palette for block in BLOCKS}
    if used_palettes != set(PALETTE):
        raise ValueError(f"Palette drift: used={sorted(used_palettes)}, defined={sorted(PALETTE)}")
    if {block_part(block) for block in BLOCKS} != set(PART_PIVOTS):
        raise ValueError("Thorgrim must retain separate body, axe, and shield parts")

    if not 4_000 <= len(voxels) <= 10_000:
        raise ValueError(f"Unexpected fixed-grid surface voxel count: {len(voxels)}")
    if {voxel.palette for voxel in voxels} != set(PALETTE):
        raise ValueError("Fixed-grid conversion lost a palette")
    if {voxel.part for voxel in voxels} != set(PART_PIVOTS):
        raise ValueError("Fixed-grid conversion lost a character part")

    vertices = np.concatenate([block_vertices(block) * MODEL_SCALE for block in BLOCKS])
    minimum = vertices.min(axis=0)
    maximum = vertices.max(axis=0)
    if minimum[2] < -0.1 or maximum[2] > 166.0:
        raise ValueError(f"Unexpected authored height: {minimum[2]:.2f} cm to {maximum[2]:.2f} cm")


FACES = (
    (0, 3, 2, 1),
    (4, 5, 6, 7),
    (0, 4, 7, 3),
    (1, 2, 6, 5),
    (0, 1, 5, 4),
    (3, 7, 6, 2),
)


def write_cpp(voxels: list[VoxelCell]) -> None:
    lines = [
        "// Generated by SourceAssets/Characters/thorgrim/generate_thorgrim.py. Do not edit by hand.",
        f"static constexpr float GThorgrimVoxelUnitCm = {VOXEL_UNIT_CM:.1f}f;",
        "static const FVector GThorgrimAxePivot(%.1ff, %.1ff, %.1ff);"
        % RUNTIME_PART_PIVOTS["Axe"],
        "static const FVector GThorgrimShieldPivot(%.1ff, %.1ff, %.1ff);"
        % RUNTIME_PART_PIVOTS["Shield"],
        "",
        "static const FThorgrimVoxelCell GThorgrimVoxelCells[] =",
        "{",
    ]
    for voxel in voxels:
        pivot = RUNTIME_PART_PIVOTS[voxel.part]
        pivot_cells = tuple(round(value / VOXEL_UNIT_CM) for value in pivot)
        x = voxel.x - pivot_cells[0]
        y = voxel.y - pivot_cells[1]
        z = voxel.z - pivot_cells[2]
        lines.append(
            "\t{ EThorgrimPart::%s, EThorgrimPalette::%s, %d, %d, %d },"
            % (voxel.part, voxel.palette, x, y, z)
        )
    lines.extend(("};", f"static constexpr int32 GThorgrimVoxelCount = {len(voxels)};", ""))
    CPP_OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    CPP_OUTPUT.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def write_obj(voxels: list[VoxelCell]) -> None:
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    obj_lines = [
        f"# Thorgrim fixed-grid model ({VOXEL_UNIT_CM:.1f} cm voxels)",
        f"mtllib {MTL_OUTPUT.name}",
    ]
    vertex_offset = 1
    for index, voxel in enumerate(voxels):
        vertices = voxel_vertices(voxel)
        obj_lines.extend((f"o voxel_{index:05d}", f"usemtl {voxel.palette}"))
        obj_lines.extend(f"v {x / 100:.6f} {y / 100:.6f} {z / 100:.6f}" for x, y, z in vertices)
        for face in FACES:
            indices = " ".join(str(vertex_offset + index) for index in face)
            obj_lines.append(f"f {indices}")
        vertex_offset += 8
    OBJ_OUTPUT.write_text("\n".join(obj_lines) + "\n", encoding="utf-8", newline="\n")

    mtl_lines = ["# Thorgrim flat palette"]
    for name, hex_color in PALETTE.items():
        red, green, blue = (int(hex_color[index : index + 2], 16) / 255.0 for index in (1, 3, 5))
        mtl_lines.extend((f"newmtl {name}", f"Kd {red:.6f} {green:.6f} {blue:.6f}", "Ka 0.000000 0.000000 0.000000", "Ks 0.050000 0.050000 0.050000", "Ns 8.000000", ""))
    MTL_OUTPUT.write_text("\n".join(mtl_lines), encoding="utf-8", newline="\n")


def shade(hex_color: str, factor: float) -> tuple[int, int, int]:
    values = [int(hex_color[index : index + 2], 16) for index in (1, 3, 5)]
    return tuple(max(0, min(255, round(value * factor))) for value in values)


def render_preview(voxels: list[VoxelCell]) -> None:
    logical_size = 560
    image = Image.new("RGB", (logical_size, logical_size), "#090C10")
    draw = ImageDraw.Draw(image)

    # Ground plate and a restrained warm/cold atmosphere.
    draw.ellipse((105, 445, 455, 510), fill="#080A0D", outline="#26211D", width=2)
    target = np.array((0.0, 0.0, 86.0))
    camera = target + np.array((390.0, 520.0, 330.0))
    forward = target - camera
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, np.array((0.0, 0.0, 1.0)))
    right /= np.linalg.norm(right)
    camera_up = np.cross(right, forward)
    light = np.array((0.7, 0.4, 1.0))
    light /= np.linalg.norm(light)

    faces_to_draw: list[tuple[float, list[tuple[float, float]], tuple[int, int, int]]] = []
    projected_points: list[tuple[float, float]] = []
    raw_faces: list[tuple[float, np.ndarray, str, float]] = []
    for voxel in voxels:
        vertices = voxel_vertices(voxel)
        for face in FACES:
            face_vertices = vertices[list(face)]
            edge_a = face_vertices[1] - face_vertices[0]
            edge_b = face_vertices[2] - face_vertices[0]
            normal = np.cross(edge_a, edge_b)
            normal_length = np.linalg.norm(normal)
            if normal_length == 0:
                continue
            normal /= normal_length
            face_center = face_vertices.mean(axis=0)
            if np.dot(normal, camera - face_center) <= 0:
                continue
            depth = float(np.dot(face_center - camera, forward))
            brightness = 0.62 + 0.38 * max(0.0, float(np.dot(normal, light)))
            points = np.column_stack(((face_vertices - target) @ right, (face_vertices - target) @ camera_up))
            raw_faces.append((depth, points, voxel.palette, brightness))
            projected_points.extend((float(point[0]), float(point[1])) for point in points)

    xs = [point[0] for point in projected_points]
    ys = [point[1] for point in projected_points]
    scale = min(390 / (max(xs) - min(xs)), 400 / (max(ys) - min(ys)))
    center_x = (min(xs) + max(xs)) / 2
    center_y = (min(ys) + max(ys)) / 2
    for depth, points, palette, brightness in raw_faces:
        screen_points = [
            (280 + (float(point[0]) - center_x) * scale, 270 - (float(point[1]) - center_y) * scale)
            for point in points
        ]
        faces_to_draw.append((depth, screen_points, shade(PALETTE[palette], brightness)))

    for _, points, color in sorted(faces_to_draw, key=lambda item: item[0], reverse=True):
        draw.polygon(points, fill=color, outline="#111419")

    font = ImageFont.load_default()
    draw.text((20, 18), "THORGRIM  /  FIXED-GRID RUNTIME MODEL", fill="#D2C2A2", font=font)
    draw.text(
        (20, 36),
        f"{len(voxels)} visible {VOXEL_UNIT_CM:.0f} cm voxels  |  165 cm target",
        fill="#777572",
        font=font,
    )
    swatch_x = 20
    for name, color in PALETTE.items():
        draw.rectangle((swatch_x, 516, swatch_x + 34, 530), fill=color, outline="#34383D")
        draw.text((swatch_x, 536), name[:5].upper(), fill="#777572", font=font)
        swatch_x += 57

    # Preserve deliberate pixels by enlarging with nearest-neighbour filtering.
    image.resize((logical_size * 2, logical_size * 2), Image.Resampling.NEAREST).save(PREVIEW_OUTPUT)


def main() -> None:
    build_model()
    voxels = voxelize_model()
    validate_model(voxels)
    write_cpp(voxels)
    write_obj(voxels)
    render_preview(voxels)
    print(f"Generated Thorgrim with {len(voxels)} visible {VOXEL_UNIT_CM:.1f} cm voxels")
    print(CPP_OUTPUT)
    print(OBJ_OUTPUT)
    print(PREVIEW_OUTPUT)


if __name__ == "__main__":
    main()
