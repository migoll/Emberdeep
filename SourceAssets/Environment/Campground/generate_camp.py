"""Build the Broken Caravan Camp from a CC0 MagicaVoxel kit.

Source shapes may be compacted while composing the layout, but the final visible
geometry is always rasterized onto Emberdeep's shared four-centimetre voxel grid.
The generated runtime data stores compressed grid rectangles and expands them
back to uniform visible voxel instances. No network access or DCC application is
required; the checked-in ``models.vox`` file is the only input.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from collections import Counter
from io import BytesIO
from pathlib import Path
import math
import struct

from PIL import Image, ImageDraw


REPO_ROOT = Path(__file__).resolve().parents[3]
ASSET_DIR = Path(__file__).resolve().parent
VOX_INPUT = ASSET_DIR / "ThirdParty" / "fuzzyman_medieval_voxels" / "models.vox"
CPP_OUTPUT = REPO_ROOT / "Source" / "Emberdeep" / "Private" / "Environment" / "CampVoxelData.inl"
PREVIEW_OUTPUT = ASSET_DIR / "broken_caravan_camp_v0_preview.png"

# Restricted palette used by both the generated C++ and the preview.  Runtime
# material mapping: Ground*=charcoal floor, Stone*=masonry, Wood*=timber,
# Canvas*=cold cloth, Iron=metal, Frost=snow edge dressing, Ember/Fire=emissive.
PALETTE: dict[str, str] = {
    "GroundDark": "#17191D",
    "GroundMid": "#28282A",
    "StoneDark": "#34363A",
    "Stone": "#56585B",
    "WoodDark": "#35251C",
    "Wood": "#5A3A22",
    "WoodLight": "#8A5728",
    "CanvasDark": "#28343B",
    "Canvas": "#4B5A5E",
    "Iron": "#73777B",
    "Frost": "#AAB7C1",
    "Ember": "#B44716",
    "Fire": "#F2A13A",
}

VOXEL_UNIT_CM = 4.0
NEIGHBOURS = (
    (1, 0, 0),
    (-1, 0, 0),
    (0, 1, 0),
    (0, -1, 0),
    (0, 0, 1),
    (0, 0, -1),
)
# The orthographic camera is fixed on the -X,+Y diagonal. Opposite-facing shell
# cells are omitted internally, while all visible faces and palette boundaries
# remain exact uniform voxels.
CAMERA_SHELL_DIRECTIONS = ((0, 0, 1), (-1, 0, 0), (0, 1, 0))


@dataclass(frozen=True)
class VoxelModel:
    model_id: int
    name: str
    size: tuple[int, int, int]
    voxels: tuple[tuple[int, int, int, int], ...]


@dataclass(frozen=True)
class Block:
    group: str
    palette: str
    center: tuple[float, float, float]
    size: tuple[float, float, float]
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)


@dataclass(frozen=True)
class VoxelCell:
    palette: str
    x: int
    y: int
    z: int


@dataclass(frozen=True)
class VoxelRect:
    palette: str
    x: int
    y: int
    z: int
    size_x: int
    size_y: int


@dataclass(frozen=True)
class CollisionBox:
    name: str
    center: tuple[float, float, float]
    size: tuple[float, float, float]
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0)


class VoxReader:
    """Small MagicaVoxel v150 reader for models and nTRN scene names."""

    def __init__(self, data: bytes) -> None:
        self.data = data
        self.offset = 0
        self.pending_size: tuple[int, int, int] | None = None
        self.models: list[tuple[tuple[int, int, int], tuple[tuple[int, int, int, int], ...]]] = []
        self.palette: list[tuple[int, int, int, int]] | None = None
        self.transforms: dict[int, tuple[str, int]] = {}
        self.groups: dict[int, tuple[int, ...]] = {}
        self.shapes: dict[int, tuple[int, ...]] = {}

    def read(self, count: int) -> bytes:
        result = self.data[self.offset : self.offset + count]
        if len(result) != count:
            raise ValueError("Unexpected end of VOX file")
        self.offset += count
        return result

    def int32(self) -> int:
        return struct.unpack("<i", self.read(4))[0]

    def dictionary(self) -> dict[str, str]:
        result: dict[str, str] = {}
        for _ in range(self.int32()):
            key = self.read(self.int32()).decode("utf-8", errors="replace")
            value = self.read(self.int32()).decode("utf-8", errors="replace")
            result[key] = value
        return result

    def parse(self) -> tuple[list[VoxelModel], list[tuple[int, int, int, int]]]:
        if self.read(4) != b"VOX ":
            raise ValueError("Not a MagicaVoxel file")
        version = self.int32()
        if version != 150:
            raise ValueError(f"Expected MagicaVoxel v150, got {version}")
        if self.read(4) != b"MAIN":
            raise ValueError("VOX MAIN chunk missing")
        content_size = self.int32()
        children_size = self.int32()
        self.offset += content_size
        self.parse_region(self.offset + children_size)

        if self.palette is None:
            raise ValueError("This source pack must contain an RGBA palette")
        names = self.resolve_model_names()
        result = [
            VoxelModel(index, names.get(index, f"Model {index}"), size, voxels)
            for index, (size, voxels) in enumerate(self.models)
        ]
        return result, self.palette

    def parse_region(self, end: int) -> None:
        while self.offset < end:
            chunk_id = self.read(4).decode("ascii", errors="replace")
            content_size = self.int32()
            children_size = self.int32()
            content_end = self.offset + content_size
            children_end = content_end + children_size
            self.parse_chunk(chunk_id, content_end)
            self.offset = content_end
            if children_size:
                self.parse_region(children_end)
            self.offset = children_end
        if self.offset != end:
            raise ValueError("Malformed VOX chunk tree")

    def parse_chunk(self, chunk_id: str, content_end: int) -> None:
        if chunk_id == "SIZE":
            self.pending_size = (self.int32(), self.int32(), self.int32())
        elif chunk_id == "XYZI":
            if self.pending_size is None:
                raise ValueError("XYZI chunk found before SIZE")
            count = self.int32()
            voxels = tuple(struct.unpack("<BBBB", self.read(4)) for _ in range(count))
            self.models.append((self.pending_size, voxels))
            self.pending_size = None
        elif chunk_id == "RGBA":
            self.palette = [struct.unpack("<BBBB", self.read(4)) for _ in range(256)]
        elif chunk_id == "nTRN":
            node_id = self.int32()
            attributes = self.dictionary()
            child_id = self.int32()
            self.int32()  # reserved id
            self.int32()  # layer id
            for _ in range(self.int32()):
                self.dictionary()  # frame attributes; asset extraction uses local model coordinates
            self.transforms[node_id] = (attributes.get("_name", ""), child_id)
        elif chunk_id == "nGRP":
            node_id = self.int32()
            self.dictionary()
            self.groups[node_id] = tuple(self.int32() for _ in range(self.int32()))
        elif chunk_id == "nSHP":
            node_id = self.int32()
            self.dictionary()
            model_ids: list[int] = []
            for _ in range(self.int32()):
                model_ids.append(self.int32())
                self.dictionary()
            self.shapes[node_id] = tuple(model_ids)
        self.offset = content_end

    def resolve_model_names(self) -> dict[int, str]:
        def descendants(node_id: int, seen: frozenset[int] = frozenset()) -> tuple[int, ...]:
            if node_id in seen:
                return ()
            seen = seen | {node_id}
            if node_id in self.shapes:
                return self.shapes[node_id]
            if node_id in self.transforms:
                return descendants(self.transforms[node_id][1], seen)
            result: list[int] = []
            for child in self.groups.get(node_id, ()):
                result.extend(descendants(child, seen))
            return tuple(result)

        names: dict[int, str] = {}
        # Direct asset transforms occur later than broad collection transforms.
        for node_id in sorted(self.transforms):
            name, child = self.transforms[node_id]
            if not name:
                continue
            for model_id in descendants(child):
                names[model_id] = name
        return names


def rgb(hex_color: str) -> tuple[int, int, int]:
    return tuple(int(hex_color[index : index + 2], 16) for index in (1, 3, 5))


TARGET_RGB = {name: rgb(value) for name, value in PALETTE.items()}


def remap_color(source: tuple[int, int, int, int]) -> str:
    """Map source RGBA to the nearest restricted camp color deterministically."""

    red, green, blue, _ = source
    # Bright warm colors must survive as flame, not become pale wood or frost.
    if red > 185 and red > green * 1.15 and green > blue * 1.35:
        return "Fire" if green > 105 else "Ember"
    candidates = tuple(name for name in PALETTE if name not in {"CanvasDark", "Canvas", "Frost", "Ember", "Fire"})
    return min(
        candidates,
        key=lambda name: sum((channel - target) ** 2 for channel, target in zip((red, green, blue), TARGET_RGB[name])),
    )


def greedy_merge(
    model: VoxelModel,
    source_palette: list[tuple[int, int, int, int]],
    coarsen: int = 1,
) -> list[tuple[str, tuple[int, int, int], tuple[int, int, int]]]:
    """Greedily merge +X, then +Y, then +Z runs of equal remapped voxels."""

    source_grid = {
        (x, y, z): remap_color(source_palette[(color_index - 1) % 256])
        for x, y, z, color_index in model.voxels
    }
    if coarsen < 1:
        raise ValueError("Coarsening factor must be positive")
    if coarsen == 1:
        grid = source_grid
    else:
        buckets: dict[tuple[int, int, int], Counter[str]] = {}
        for (x, y, z), palette in source_grid.items():
            buckets.setdefault((x // coarsen, y // coarsen, z // coarsen), Counter())[palette] += 1
        palette_order = {name: index for index, name in enumerate(PALETTE)}
        grid = {
            cell: min(counts, key=lambda name: (-counts[name], palette_order[name]))
            for cell, counts in buckets.items()
        }
    visited: set[tuple[int, int, int]] = set()
    merged: list[tuple[str, tuple[int, int, int], tuple[int, int, int]]] = []
    for seed in sorted(grid, key=lambda value: (value[2], value[1], value[0])):
        if seed in visited:
            continue
        x0, y0, z0 = seed
        palette = grid[seed]
        x1 = x0
        while grid.get((x1 + 1, y0, z0)) == palette and (x1 + 1, y0, z0) not in visited:
            x1 += 1

        y1 = y0
        while all(
            grid.get((x, y1 + 1, z0)) == palette and (x, y1 + 1, z0) not in visited
            for x in range(x0, x1 + 1)
        ):
            y1 += 1

        z1 = z0
        while all(
            grid.get((x, y, z1 + 1)) == palette and (x, y, z1 + 1) not in visited
            for y in range(y0, y1 + 1)
            for x in range(x0, x1 + 1)
        ):
            z1 += 1

        for z in range(z0, z1 + 1):
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    visited.add((x, y, z))
        merged.append((palette, (x0, y0, z0), (x1 + 1, y1 + 1, z1 + 1)))
    return merged


class CampBuilder:
    def __init__(self, models: list[VoxelModel], source_palette: list[tuple[int, int, int, int]]) -> None:
        self.models = {model.model_id: model for model in models}
        self.source_palette = source_palette
        self.blocks: list[Block] = []
        self.collisions: list[CollisionBox] = []
        self.selected: list[tuple[int, str, int]] = []

    def box(
        self,
        group: str,
        palette: str,
        center: tuple[float, float, float],
        size: tuple[float, float, float],
        yaw: float = 0.0,
    ) -> None:
        if palette not in PALETTE or min(size) <= 0.0:
            raise ValueError(f"Invalid custom camp block: {group}/{palette}/{size}")
        self.blocks.append(Block(group, palette, center, size, (0.0, yaw, 0.0)))

    def model(
        self,
        model_id: int,
        group: str,
        location: tuple[float, float, float],
        voxel_cm: float,
        yaw: float = 0.0,
        expected_name: str = "",
        coarsen: int = 1,
    ) -> None:
        model = self.models[model_id]
        normalized_actual = "".join(character for character in model.name.casefold() if character.isalnum())
        normalized_expected = "".join(character for character in expected_name.casefold() if character.isalnum())
        if normalized_expected and normalized_expected not in normalized_actual:
            raise ValueError(f"Model {model_id} is {model.name!r}, expected {expected_name!r}")
        cuboids = greedy_merge(model, self.source_palette, coarsen)
        size_x, size_y, _ = model.size
        radians = math.radians(yaw)
        cosine, sine = math.cos(radians), math.sin(radians)
        for palette, minimum, maximum in cuboids:
            x0, y0, z0 = minimum
            x1, y1, z1 = maximum
            x0, y0, z0 = (value * coarsen for value in (x0, y0, z0))
            x1 = min(x1 * coarsen, model.size[0])
            y1 = min(y1 * coarsen, model.size[1])
            z1 = min(z1 * coarsen, model.size[2])
            local_x = ((x0 + x1) / 2.0 - size_x / 2.0) * voxel_cm
            local_y = ((y0 + y1) / 2.0 - size_y / 2.0) * voxel_cm
            world_x = location[0] + local_x * cosine - local_y * sine
            world_y = location[1] + local_x * sine + local_y * cosine
            world_z = location[2] + z0 * voxel_cm + (z1 - z0) * voxel_cm / 2.0
            self.blocks.append(
                Block(
                    group,
                    palette,
                    (world_x, world_y, world_z),
                    ((x1 - x0) * voxel_cm, (y1 - y0) * voxel_cm, (z1 - z0) * voxel_cm),
                    (0.0, yaw, 0.0),
                )
            )
        self.selected.append((model_id, model.name, len(cuboids)))

    def build_ground(self) -> None:
        # 18 x 14 metres. Small deterministic offsets stop the floor looking tiled.
        for ix in range(18):
            for iy in range(14):
                x = -850.0 + ix * 100.0 + ((ix * 17 + iy * 11) % 9 - 4)
                y = -650.0 + iy * 100.0 + ((ix * 7 + iy * 19) % 9 - 4)
                selector = (ix * 13 + iy * 29) % 11
                palette = "GroundMid" if selector in (0, 5) else "GroundDark"
                if selector == 8:
                    palette = "StoneDark"
                self.box("Ground", palette, (x, y, -1.0 + selector % 3), (94.0, 94.0, 10.0))

        # Sparse frost around the boundary; the warm playable centre stays readable.
        for index in range(64):
            side = index % 4
            along = ((index * 137) % 1500) - 750
            inset = (index * 31) % 75
            if side < 2:
                x = along
                y = (-1.0 if side == 0 else 1.0) * (620.0 - inset)
            else:
                x = (-1.0 if side == 2 else 1.0) * (820.0 - inset)
                y = along * 0.78
            size = 24.0 + (index * 17) % 42
            self.box("Frost", "Frost", (x, y, 7.0), (size, size * (0.55 + (index % 3) * 0.18), 5.0))

    def build_custom_shelter(self) -> None:
        # Salvaged canvas lean-to at the west edge, coarse enough to remain voxel-like.
        origin_x, origin_y = -535.0, 315.0
        for x in (-655.0, -415.0):
            for y in (245.0, 395.0):
                self.box("Shelter", "WoodDark", (x, y, 95.0), (20.0, 20.0, 180.0))
        for strip in range(7):
            x = origin_x - 108.0 + strip * 36.0
            z = 178.0 - abs(strip - 3) * 7.0
            self.box("Shelter", "CanvasDark" if strip % 3 else "Canvas", (x, origin_y, z), (38.0, 190.0, 14.0), 0.0)
        self.box("Shelter", "Wood", (origin_x, 410.0, 55.0), (250.0, 24.0, 36.0))
        self.box("Shelter", "CanvasDark", (origin_x, 412.0, 92.0), (210.0, 10.0, 48.0))

    def build_custom_wagon_canopy(self) -> None:
        """Give the source wagon the covered silhouette used by the camp concept."""

        origin_x, origin_y = 470.0, 390.0
        yaw = -12.0
        radians = math.radians(yaw)
        cosine, sine = math.cos(radians), math.sin(radians)

        def world(local_x: float, local_y: float, z: float) -> tuple[float, float, float]:
            return (
                origin_x + local_x * cosine - local_y * sine,
                origin_y + local_x * sine + local_y * cosine,
                z,
            )

        for local_x in (-62.0, 62.0):
            for local_y in (-135.0, 135.0):
                self.box("WagonCanopy", "WoodDark", world(local_x, local_y, 146.0), (18.0, 18.0, 122.0), yaw)

        # Five stepped strips at each cross-section suggest a curved canvas roof
        # while preserving the deliberately block-built silhouette.
        for segment, local_y in enumerate((-144.0, -96.0, -48.0, 0.0, 48.0, 96.0, 144.0)):
            accent = "Canvas" if segment in (1, 5) else "CanvasDark"
            self.box("WagonCanopy", accent, world(0.0, local_y, 226.0), (62.0, 52.0, 16.0), yaw)
            self.box("WagonCanopy", "Canvas", world(-46.0, local_y, 211.0), (40.0, 52.0, 18.0), yaw)
            self.box("WagonCanopy", "Canvas", world(46.0, local_y, 211.0), (40.0, 52.0, 18.0), yaw)
            self.box("WagonCanopy", "CanvasDark", world(-70.0, local_y, 187.0), (24.0, 52.0, 38.0), yaw)
            self.box("WagonCanopy", "CanvasDark", world(70.0, local_y, 187.0), (24.0, 52.0, 38.0), yaw)

    def build_custom_dressing(self) -> None:
        # Central flame and a few deliberate canvas accents create the mood even
        # before particles and point lights are attached by the runtime actor.
        self.box("CentralFire", "Ember", (-12.0, 4.0, 42.0), (30.0, 30.0, 70.0))
        self.box("CentralFire", "Fire", (12.0, -8.0, 58.0), (24.0, 24.0, 92.0))
        self.box("CentralFire", "Fire", (-4.0, 14.0, 92.0), (18.0, 18.0, 66.0))
        self.box("CentralFire", "Ember", (18.0, 11.0, 27.0), (28.0, 20.0, 38.0))

        # Two banners mark the northern wall and frame the wagon.
        for x in (185.0, 335.0):
            self.box("Banners", "WoodDark", (x, 565.0, 105.0), (18.0, 18.0, 200.0))
            self.box("Banners", "CanvasDark", (x + 38.0, 563.0, 135.0), (72.0, 12.0, 105.0))
            self.box("Banners", "Ember", (x + 38.0, 555.0, 135.0), (22.0, 6.0, 22.0))

        # Lantern cores: runtime lights use these same positions.
        for x, y, z in ((390.0, 455.0, 150.0), (-405.0, 235.0, 132.0), (520.0, -350.0, 126.0)):
            self.box("Lanterns", "Iron", (x, y, z), (28.0, 28.0, 42.0))
            self.box("Lanterns", "Fire", (x, y, z), (14.0, 14.0, 24.0))

    def build(self) -> None:
        self.build_ground()

        # Signature concept landmarks.
        self.model(323, "CentralFire", (0.0, 0.0, 5.0), 9.0, 0.0, "Fire Ring", 2)
        self.model(98, "CentralFire", (0.0, 0.0, 10.0), 9.0, 25.0, "Fire Log")
        self.model(207, "Wagon", (470.0, 390.0, 5.0), 10.5, -12.0, "Waggon", 2)
        self.model(206, "Wagon", (405.0, 250.0, 12.0), 10.5, -12.0, "Waggon Wheel")
        self.build_custom_wagon_canopy()
        self.model(177, "Shelter", (-535.0, 315.0, 7.0), 10.0, 0.0, "Shelter Wooden Roof", 4)
        self.build_custom_shelter()

        # Work and rest areas around the open ring.
        self.model(202, "Workstation", (535.0, -340.0, 6.0), 9.0, -18.0, "Sharpening Wheel")
        self.model(203, "Workstation", (535.0, -340.0, 6.0), 9.0, -18.0, "Frame", 3)
        self.model(24, "Workstation", (585.0, -460.0, 5.0), 9.0, -18.0, "Table Plain Dark", 2)
        self.model(281, "Cooking", (-280.0, -275.0, 5.0), 8.5, 10.0, "Cauldron", 3)
        for index, (x, y, yaw) in enumerate(((-235.0, 150.0, 30.0), (220.0, 150.0, -30.0), (245.0, -145.0, 25.0))):
            self.model(276, f"Bench{index}", (x, y, 5.0), 8.5, yaw, "Bench", 3)

        # Storage clusters are concentrated beside structures, not in movement lanes.
        for index, (model_id, name, x, y, yaw) in enumerate(
            (
                (259, "Slat Crate", 670.0, 325.0, -8.0),
                (259, "Slat Crate", 650.0, 220.0, 10.0),
                (285, "Barrel", 680.0, 445.0, 0.0),
                (285, "Barrel", -690.0, 350.0, 0.0),
                (259, "Slat Crate", -675.0, 245.0, 6.0),
                (285, "Barrel", 690.0, -420.0, 0.0),
            )
        ):
            self.model(model_id, f"Storage{index}", (x, y, 5.0), 8.0, yaw, name, 3)

        # Low broken fencing suggests a refuge while leaving a broad southern
        # approach around the player spawn. A separate invisible arena boundary
        # keeps this Phase 0A combat slice contained.
        for index, (x, y, yaw) in enumerate(
            (
                (-610.0, 545.0, 0.0), (-250.0, 585.0, 0.0), (570.0, 560.0, 0.0),
                (-790.0, 160.0, 90.0), (-790.0, -260.0, 90.0),
                (790.0, 160.0, 90.0), (790.0, -250.0, 90.0),
                (-580.0, -580.0, 0.0), (565.0, -575.0, 0.0),
            )
        ):
            self.model(283, f"Fence{index}", (x, y, 5.0), 8.0, yaw, "Fence Straight", 3)

        # Edge silhouettes and stone scatter strengthen the cold exterior.
        for index, (x, y, scale, yaw) in enumerate(
            ((-740.0, 535.0, 7.0, 0.0), (730.0, 545.0, 6.5, 15.0))
        ):
            self.model(334, f"Tree{index}", (x, y, 2.0), scale, yaw, "Tree", 4)
        stone_ids = (318, 319, 320, 321)
        for index in range(12):
            angle = math.radians((index * 137) % 360)
            radius_x = 650.0 + (index * 29) % 125
            radius_y = 470.0 + (index * 43) % 110
            self.model(
                stone_ids[index % len(stone_ids)],
                f"Stone{index}",
                (math.cos(angle) * radius_x, math.sin(angle) * radius_y, 4.0),
                5.0 + (index % 3),
                float((index * 47) % 360),
                "Stone",
            )

        self.build_custom_dressing()

        self.collisions.extend(
            (
                CollisionBox("Central fire", (0.0, 0.0, 55.0), (155.0, 155.0, 110.0)),
                CollisionBox("Wagon", (464.8, 365.5, 120.0), (225.0, 410.0, 240.0), (0.0, -12.0, 0.0)),
                CollisionBox("Shelter", (-535.0, 335.0, 90.0), (300.0, 205.0, 180.0)),
                CollisionBox("Workstation", (560.0, -370.0, 62.0), (245.0, 145.0, 124.0), (0.0, -18.0, 0.0)),
                CollisionBox("Cooking pot", (-280.0, -275.0, 55.0), (105.0, 105.0, 110.0), (0.0, 10.0, 0.0)),
            )
        )


def rasterize_blocks(blocks: list[Block]) -> dict[tuple[int, int, int], str]:
    """Snap authored cuboids and rotations onto the one world-space lattice."""

    occupied: dict[tuple[int, int, int], str] = {}
    for block in blocks:
        center_x, center_y, center_z = block.center
        size_x, size_y, size_z = block.size
        half_x, half_y, half_z = size_x / 2.0, size_y / 2.0, size_z / 2.0
        yaw = math.radians(block.rotation[1])
        inverse_cosine = math.cos(-yaw)
        inverse_sine = math.sin(-yaw)
        extent_x = abs(math.cos(yaw)) * half_x + abs(math.sin(yaw)) * half_y
        extent_y = abs(math.sin(yaw)) * half_x + abs(math.cos(yaw)) * half_y
        wrote_voxel = False

        x_range = range(
            math.floor((center_x - extent_x) / VOXEL_UNIT_CM) - 1,
            math.floor((center_x + extent_x) / VOXEL_UNIT_CM) + 2,
        )
        y_range = range(
            math.floor((center_y - extent_y) / VOXEL_UNIT_CM) - 1,
            math.floor((center_y + extent_y) / VOXEL_UNIT_CM) + 2,
        )
        z_range = range(
            math.floor((center_z - half_z) / VOXEL_UNIT_CM) - 1,
            math.floor((center_z + half_z) / VOXEL_UNIT_CM) + 2,
        )
        for x in x_range:
            sample_x = (x + 0.5) * VOXEL_UNIT_CM
            for y in y_range:
                sample_y = (y + 0.5) * VOXEL_UNIT_CM
                delta_x = sample_x - center_x
                delta_y = sample_y - center_y
                local_x = delta_x * inverse_cosine - delta_y * inverse_sine
                local_y = delta_x * inverse_sine + delta_y * inverse_cosine
                if abs(local_x) > half_x + 1e-5 or abs(local_y) > half_y + 1e-5:
                    continue
                for z in z_range:
                    sample_z = (z + 0.5) * VOXEL_UNIT_CM
                    if abs(sample_z - center_z) <= half_z + 1e-5:
                        occupied[(x, y, z)] = block.palette
                        wrote_voxel = True

        if not wrote_voxel:
            occupied[
                tuple(math.floor(value / VOXEL_UNIT_CM) for value in block.center)
            ] = block.palette
    return occupied


def visible_voxels(occupied: dict[tuple[int, int, int], str]) -> list[VoxelCell]:
    visible: list[VoxelCell] = []
    for (x, y, z), palette in occupied.items():
        visible_from_camera = any(
            (x + dx, y + dy, z + dz) not in occupied
            for dx, dy, dz in CAMERA_SHELL_DIRECTIONS
        )
        if visible_from_camera:
            visible.append(VoxelCell(palette, x, y, z))

    palette_order = {name: index for index, name in enumerate(PALETTE)}
    return sorted(
        visible,
        key=lambda voxel: (palette_order[voxel.palette], voxel.z, voxel.y, voxel.x),
    )


def compress_voxels(voxels: list[VoxelCell]) -> list[VoxelRect]:
    """Compress each Z/palette layer into rectangles without changing cells."""

    layers: dict[tuple[str, int], set[tuple[int, int]]] = {}
    for voxel in voxels:
        layers.setdefault((voxel.palette, voxel.z), set()).add((voxel.x, voxel.y))

    rectangles: list[VoxelRect] = []
    palette_order = {name: index for index, name in enumerate(PALETTE)}
    for (palette, z), source_cells in sorted(
        layers.items(), key=lambda item: (palette_order[item[0][0]], item[0][1])
    ):
        remaining = set(source_cells)
        while remaining:
            x0, y0 = min(remaining, key=lambda cell: (cell[1], cell[0]))
            x1 = x0
            while (x1 + 1, y0) in remaining:
                x1 += 1

            y1 = y0
            while all((x, y1 + 1) in remaining for x in range(x0, x1 + 1)):
                y1 += 1

            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    remaining.remove((x, y))
            rectangles.append(VoxelRect(palette, x0, y0, z, x1 - x0 + 1, y1 - y0 + 1))
    return rectangles


def fmt(value: float) -> str:
    if abs(value) < 0.0005:
        value = 0.0
    return f"{value:.1f}f"


def cpp_text(builder: CampBuilder, rectangles: list[VoxelRect], voxel_count: int) -> str:
    lines = [
        "// Generated by SourceAssets/Environment/Campground/generate_camp.py. Do not edit by hand.",
        "// Palette colors are documented beside PALETTE in the generator and Campground README.",
        "enum class ECampPalette : uint8",
        "{",
    ]
    lines.extend(f"\t{name}," for name in PALETTE)
    lines.extend(
        (
            "};",
            "",
            f"static constexpr float GCampVoxelUnitCm = {VOXEL_UNIT_CM:.1f}f;",
            "",
            "struct FCampVoxelRect",
            "{",
            "\tECampPalette Palette;",
            "\tint16 X;",
            "\tint16 Y;",
            "\tint16 Z;",
            "\tuint16 SizeX;",
            "\tuint16 SizeY;",
            "};",
            "",
            "struct FCampCollisionBox",
            "{",
            "\tFVector Center;",
            "\tFVector Size;",
            "\tFRotator Rotation;",
            "};",
            "",
            "static const FVector GCampPrimaryFireLocation(0.0f, 0.0f, 145.0f);",
            "static const FVector GCampWagonLanternLocation(390.0f, 455.0f, 150.0f);",
            "static const FVector GCampShelterLanternLocation(-405.0f, 235.0f, 132.0f);",
            "static const FVector GCampWorkstationLanternLocation(520.0f, -350.0f, 126.0f);",
            "",
            "static const FCampVoxelRect GCampVoxelRects[] =",
            "{",
        )
    )
    previous_palette = ""
    for rectangle in rectangles:
        if rectangle.palette != previous_palette:
            lines.append(f"\t// {rectangle.palette}")
            previous_palette = rectangle.palette
        lines.append(
            "\t{ ECampPalette::%s, %d, %d, %d, %d, %d },"
            % (
                rectangle.palette,
                rectangle.x,
                rectangle.y,
                rectangle.z,
                rectangle.size_x,
                rectangle.size_y,
            )
        )
    lines.extend(
        (
            "};",
            f"static constexpr int32 GCampVoxelCount = {voxel_count};",
            "",
            "static const FCampCollisionBox GCampCollisionBoxes[] =",
            "{",
        )
    )
    for collision in builder.collisions:
        lines.append(f"\t// {collision.name}")
        lines.append(
            "\t{ FVector(%s, %s, %s), FVector(%s, %s, %s), FRotator(%s, %s, %s) },"
            % (
                *(fmt(value) for value in collision.center),
                *(fmt(value) for value in collision.size),
                *(fmt(value) for value in collision.rotation),
            )
        )
    lines.extend(("};", ""))
    return "\n".join(lines)


def shade(color: str, factor: float) -> tuple[int, int, int]:
    return tuple(max(0, min(255, round(channel * factor))) for channel in rgb(color))


def preview_bytes(voxels: list[VoxelCell]) -> bytes:
    logical_width, logical_height = 720, 520
    image = Image.new("RGB", (logical_width, logical_height), "#090C10")
    draw = ImageDraw.Draw(image)
    palette_indices = {name: index for index, name in enumerate(PALETTE)}

    def project(point: tuple[float, float, float]) -> tuple[float, float]:
        x, y, z = point
        # Match Thorgrim's runtime camera: yaw -45 places the camera on the
        # -X,+Y diagonal, with world +X,+Y running toward screen-right.
        return (logical_width / 2.0 + (x + y) * 0.205, 310.0 + (-x + y) * 0.118 - z * 0.117)

    def center(voxel: VoxelCell) -> tuple[float, float, float]:
        return (
            (voxel.x + 0.5) * VOXEL_UNIT_CM,
            (voxel.y + 0.5) * VOXEL_UNIT_CM,
            (voxel.z + 0.5) * VOXEL_UNIT_CM,
        )

    def vertices(voxel: VoxelCell) -> list[tuple[float, float, float]]:
        center_x, center_y, center_z = center(voxel)
        half = VOXEL_UNIT_CM / 2.0
        return [
            (center_x + x, center_y + y, center_z + z)
            for x, y, z in (
                (-half, -half, -half), (half, -half, -half),
                (half, half, -half), (-half, half, -half),
                (-half, -half, half), (half, -half, half),
                (half, half, half), (-half, half, half),
            )
        ]

    def shade_index(voxel: VoxelCell) -> int:
        value = 2166136261
        for component in (voxel.x, voxel.y, voxel.z, palette_indices[voxel.palette]):
            value = ((value ^ (component & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
        selector = value % 16
        if selector < 3:
            return 0
        return 2 if selector == 15 else 1

    # Camera looks from -X,+Y. Draw far blocks first, then their visible faces.
    ordered = sorted(
        voxels,
        key=lambda voxel: (
            -center(voxel)[0] + center(voxel)[1] + center(voxel)[2] * 2.02,
            voxel.palette,
        ),
    )

    def draw_voxel(voxel: VoxelCell) -> None:
        points = vertices(voxel)
        projected = [project(point) for point in points]
        color = PALETTE[voxel.palette]
        variation = (0.72, 1.0, 1.16)[shade_index(voxel)]
        outline = shade(color, 0.30 * variation)
        draw.polygon(
            [projected[index] for index in (0, 4, 7, 3)],
            fill=shade(color, 0.58 * variation),
            outline=outline,
        )
        draw.polygon(
            [projected[index] for index in (3, 2, 6, 7)],
            fill=shade(color, 0.76 * variation),
            outline=outline,
        )
        draw.polygon(
            [projected[index] for index in (4, 5, 6, 7)],
            fill=shade(color, 1.08 * variation),
            outline=outline,
        )

    # Ground first, then translucent stepped light pools, then upright props.
    # This is a readability preview, not a substitute for Unreal's point lights.
    for voxel in ordered:
        if voxel.z <= 2:
            draw_voxel(voxel)

    light_overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
    light_draw = ImageDraw.Draw(light_overlay)
    for location, radii in (
        ((0.0, 0.0, 0.0), ((150, "#B34A16", 22), (105, "#D16B1E", 30), (58, "#F5A33A", 44))),
        ((390.0, 455.0, 4.0), ((44, "#D16B1E", 24), (23, "#F5A33A", 40))),
        ((-405.0, 235.0, 4.0), ((44, "#D16B1E", 24), (23, "#F5A33A", 40))),
        ((520.0, -350.0, 4.0), ((44, "#D16B1E", 24), (23, "#F5A33A", 40))),
    ):
        center_x, center_y = project(location)
        for radius, color, alpha in radii:
            light_draw.ellipse(
                (center_x - radius, center_y - radius * 0.48, center_x + radius, center_y + radius * 0.48),
                fill=(*rgb(color), alpha),
            )
    image = Image.alpha_composite(image.convert("RGBA"), light_overlay).convert("RGB")
    draw = ImageDraw.Draw(image)

    for voxel in ordered:
        if voxel.z > 2:
            draw_voxel(voxel)

    # Presentation frame and simple labels remain deterministic and font-independent.
    draw.rectangle((12, 12, logical_width - 13, logical_height - 13), outline="#6D4B2C", width=2)
    draw.text((28, 24), "BROKEN CARAVAN CAMP  /  PLAYABLE V0", fill="#D59A52")
    draw.text(
        (28, logical_height - 42),
        f"{len(voxels)} VISIBLE {VOXEL_UNIT_CM:.0f} CM VOXELS  |  18 x 14 METRES",
        fill="#8C8273",
    )

    final = image.resize((logical_width * 2, logical_height * 2), Image.Resampling.NEAREST)
    output = BytesIO()
    final.save(output, format="PNG", optimize=False, compress_level=9)
    return output.getvalue()


def validate(builder: CampBuilder, voxels: list[VoxelCell], rectangles: list[VoxelRect]) -> None:
    if not 500 <= len(builder.blocks) <= 1500:
        raise ValueError(f"Camp source block count {len(builder.blocks)} is outside the 500-1500 target")
    if not 150_000 <= len(voxels) <= 350_000:
        raise ValueError(f"Camp fixed-grid voxel count {len(voxels)} is outside the expected range")
    if not rectangles or len(rectangles) >= len(voxels):
        raise ValueError("Camp voxel rectangle compression is ineffective")
    if sum(rectangle.size_x * rectangle.size_y for rectangle in rectangles) != len(voxels):
        raise ValueError("Compressed camp rectangles do not preserve the visible voxel count")
    used = {voxel.palette for voxel in voxels}
    missing = set(PALETTE) - used
    if missing:
        raise ValueError(f"Unused palette entries: {sorted(missing)}")
    if len(builder.collisions) > 12:
        raise ValueError("Camp collision should stay deliberately simple")
    for block in builder.blocks:
        x, y, _ = block.center
        if abs(x) > 925.0 or abs(y) > 725.0:
            raise ValueError(f"Block outside 1800x1400 camp dressing bounds: {block}")


def generate(check: bool = False) -> tuple[CampBuilder, list[VoxelCell], list[VoxelRect]]:
    models, source_palette = VoxReader(VOX_INPUT.read_bytes()).parse()
    if len(models) != 363:
        raise ValueError(f"Expected 363 CC0 source models, found {len(models)}")
    builder = CampBuilder(models, source_palette)
    builder.build()
    occupied = rasterize_blocks(builder.blocks)
    voxels = visible_voxels(occupied)
    rectangles = compress_voxels(voxels)
    validate(builder, voxels, rectangles)

    cpp = cpp_text(builder, rectangles, len(voxels)).encode("utf-8")
    preview = preview_bytes(voxels)
    if check:
        if not CPP_OUTPUT.exists() or CPP_OUTPUT.read_bytes() != cpp:
            raise SystemExit(f"Generated output is stale: {CPP_OUTPUT}")
        if not PREVIEW_OUTPUT.exists():
            raise SystemExit(f"Generated output is stale: {PREVIEW_OUTPUT}")
        with Image.open(PREVIEW_OUTPUT) as existing_image, Image.open(BytesIO(preview)) as expected_image:
            existing_pixels = existing_image.convert("RGB")
            expected_pixels = expected_image.convert("RGB")
            if existing_pixels.size != expected_pixels.size or existing_pixels.tobytes() != expected_pixels.tobytes():
                raise SystemExit(f"Generated output is stale: {PREVIEW_OUTPUT}")
    else:
        for path, content in ((CPP_OUTPUT, cpp), (PREVIEW_OUTPUT, preview)):
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(content)
    return builder, voxels, rectangles


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail if generated outputs differ from checked-in files")
    parser.add_argument("--list-models", action="store_true", help="print all model ids, scene names, dimensions and voxel counts")
    arguments = parser.parse_args()

    if arguments.list_models:
        models, _ = VoxReader(VOX_INPUT.read_bytes()).parse()
        for model in models:
            print(f"{model.model_id:3d}  {model.name:52s}  {model.size!s:16s}  {len(model.voxels):6d} voxels")
        return

    builder, voxels, rectangles = generate(arguments.check)
    unique_sources = {(model_id, name) for model_id, name, _ in builder.selected}
    print(
        f"Camp generated: {len(voxels)} visible {VOXEL_UNIT_CM:.1f} cm voxels, "
        f"{len(rectangles)} compressed rectangles, {len(builder.collisions)} collision boxes"
    )
    print(f"Source placements: {len(builder.selected)} from {len(unique_sources)} unique CC0 models")
    for model_id, name, cuboids in sorted(set(builder.selected)):
        print(f"  {model_id:3d} {name}: {cuboids} merged cuboids")


if __name__ == "__main__":
    main()
