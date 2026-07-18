# Emberdeep

Emberdeep is a dark-fantasy cooperative action RPG built with Unreal Engine 5.8. Its visual direction combines chunky low-poly, voxel-inspired 3D geometry with a deliberately pixelated presentation.

The core loop is:

> Kill -> loot -> grow stronger -> descend deeper.

## Current phase

Phase 0A proves one Fighter, one small dungeon arena, responsive combat, one skeleton foundation, and the intended visual treatment. Multiplayer begins only in Phase 0B, immediately after the combat proof succeeds.

The current Phase 0A slice contains:

- a generated blockout dungeon arena;
- fixed orthographic near-isometric camera;
- visible chunky placeholder Fighter;
- screen-relative WASD movement and invulnerable dodge;
- basic and heavy melee attacks with cooldowns and knockback;
- three server-owned skeleton enemies with proximity aggro;
- health, death, automatic encounter restart, enemy hit flashes, and defeat feedback;
- dropped gold pickups and victory tracking;
- a placeholder combat HUD for health, dodge recovery, gold, and enemy count;
- replicated-ready C++ health and gameplay foundations.

## Controls

- `WASD`: move
- Left mouse or `Space`: basic attack hook
- Right mouse or `E`: heavy attack hook
- Left Shift: dodge hook
- `Escape`: quit standalone test

Designer handoff templates live in `SourceAssets/Characters/Fighter` and
`SourceAssets/Environment/Campground`. Runtime imports belong under the matching
`Content/Emberdeep` folders.

## Setup

1. Install Unreal Engine 5.8.
2. Install MSVC v143 x64 build tools and a Windows 11 SDK.
3. Double-click `Emberdeep.uproject` after the first successful C++ build.

Generated folders such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` are intentionally ignored.

See `ROADMAP.md`, `BACKLOG.md`, `docs/ART_DIRECTION.md`, and `docs/ASSET_PIPELINE.md` before adding systems or content.
