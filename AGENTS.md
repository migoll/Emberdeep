# Emberdeep working agreement

## Product

Emberdeep is a dark-fantasy action RPG. The long-term direction is server-authoritative 4-5 player dungeon co-op with the option to grow into larger shared social or realm zones. Early phases prove combat and visuals before networking infrastructure or content scale.

## Locked visual direction

- Technically 3D, presented like premium modern pixel art.
- Fixed near-isometric camera with an orthographic projection.
- All visible solid art uses one 4 cm fundamental voxel lattice; this is not a destructible voxel world.
- Larger forms are assembled from repeated 4 cm cells (for example, a 2x2 face), never made by stretching an individual voxel or placing arbitrary cuboids off-grid.
- Normal gameplay characters use a restrained density: author most forms as 2x2-cell modules and reserve isolated 4 cm cells for silhouette-critical accents. Thorgrim is an upper-bound experiment, not the production default.
- Characters, equipment, props, and environments share the same cell size. Animation may translate and rotate rigid voxel clusters, but must not shear or non-uniformly scale their cells.
- Glows, particles, fog, telegraphs, and other effects are the deliberate exceptions to the solid-geometry lattice.
- Oversized heads, hands, weapons, shields, and shoulder armour.
- Restricted dark-fantasy palette and strong silhouette/value separation.
- Hard-edged lighting, restrained fog, warm torch pools, and colored ability light.
- Low internal render resolution with nearest-neighbour presentation.
- Deliberately stepped character animation while gameplay remains responsive.
- Pixelated particles, readable telegraphs, minimal bloom, no motion blur, no photorealism.
- Gold is reserved for legendary rewards; crimson communicates danger.

The reference target is the approved four-hero dungeon combat concept: Diablo-like composition and UI with blocky miniature characters and a low-resolution presentation.

## Engineering rules

- Unreal Engine 5.8, C++ foundations, thin Blueprints only for asset composition and designer configuration.
- Gameplay state must remain server-authority-compatible even in the solo Phase 0 build.
- Do not implement networking in Phase 0A. Phase 0B is the mandatory 2-5 player networking proof.
- Prefer small Actor Components and explicit gameplay paths over giant classes or speculative frameworks.
- Keep visual/cosmetic feedback separate from authoritative damage and state changes.
- Do not put important reusable gameplay logic in Level Blueprints.
- Use automation tests and command-line builds for every meaningful increment.
- Source art belongs under `SourceAssets/`; runtime-ready imported content belongs under `Content/`.
- Generated Unreal folders are never committed.
- Unreal/source-art binary files use Git LFS.

## Scope discipline

Only implement the active roadmap phase. Do not add accounts, persistence, matchmaking, procedural dungeons, inventory frameworks, AI Dungeon Master integration, or backend services until their phase begins.
