# Broken Caravan Camp v0

This folder contains the deterministic source-data pipeline for Emberdeep's first
playable camp. The goal is atmosphere and spatial composition, not a finished
environment kit: a warm central fire, a broken wagon, salvaged shelter, work and
cooking areas, cold boundary dressing, and broad lanes for character movement.

![Generated camp preview](broken_caravan_camp_v0_preview.png)

## Generate

From the repository root, install the pinned preview dependency and run:

```powershell
python -m pip install -r SourceAssets/Environment/Campground/requirements.txt
python SourceAssets/Environment/Campground/generate_camp.py
python SourceAssets/Environment/Campground/generate_camp.py --check
```

The first command writes:

- `broken_caravan_camp_v0_preview.png`, a deterministic nearest-neighbour art preview;
- `Source/Emberdeep/Private/Environment/CampVoxelData.inl`, the compressed runtime cell,
  palette, light-anchor, and simple collision data.

Use `--list-models` to inspect all 363 named source models. The generator parses
MagicaVoxel v150 scene names (`nTRN`), remaps source colors to Emberdeep's restricted
palette, rasterizes all visible solids onto Emberdeep's shared 4 cm lattice, and
compresses adjacent cells into data rectangles. Unreal expands those rectangles to
256,394 identical 4 cm cube instances; compression never changes their rendered size.

## Layout and palette

The authored footprint is 1800 x 1400 cm. The central fire and a generous loop
around it remain open. Dense props live beside the wagon, shelter, and workstation.
The southern visual gap frames Thorgrim's spawn approach; an invisible boundary
keeps the Phase 0A combat arena contained. Decorative fences, benches, and small
storage remain non-blocking in v0, while the floor, perimeter, and five major props
use simple collision proxies.

Runtime colors are intentionally few: charcoal ground, two stones, three woods,
two cold canvas colors, iron, frost, ember, and fire. `Ember` and `Fire` are bright
palette accents backed by local point lights in v0; a dedicated emissive material
is reserved for the first in-editor lighting polish pass. Other surfaces stay matte
and hard-edged.

## Source models used

The composition currently uses these pack modules (some more than once):

- 24 Table Plain Dark;
- 98 Fire Log;
- 177 Shelter Wooden Roof;
- 202-203 Sharpening Wheel and Frame;
- 206-207 Transport Waggon Wheel and Transport Waggon;
- 259 Slat Crate;
- 276 Bench;
- 281 Cauldron;
- 283 Fence Straight;
- 285 Barrel;
- 318-321 Stones;
- 323 Fire Ring;
- 334 Skinny Brown Tree.

Custom source forms add the paver ground, perimeter frost, covered-wagon
canopy, canvas shelter, banners, lantern cores, and stylized flame before fixed-grid
rasterization. The primary visual reference is
`broken_caravan_camp_concept.png` in this folder.

## Third-party provenance and license

`ThirdParty/fuzzyman_medieval_voxels/models.vox` is from **Medieval Theme Voxels
Asset Pack** by **FuzzyManStudios**, released under **CC0 / public domain**:

<https://opengameart.org/content/medieval-theme-voxels-asset-pack>

The original file is retained unmodified for reproducibility. Attribution is not
required by CC0; the retrieval record and SHA-256 are in
`ThirdParty/fuzzyman_medieval_voxels/SOURCE.md`. Emberdeep's layout,
palette remapping, generated data, and preview are project work derived from that
source and the checked-in camp concept.

The concept is an AI-generated visual reference supplied by the project owner on
2026-07-18. Its source prompt and generation settings were not retained.
