# Emberdeep working agreement

## Product

Emberdeep is a dark-fantasy action RPG. The long-term direction is server-authoritative 4-5 player dungeon co-op with the option to grow into larger shared social or realm zones. Early phases prove combat and visuals before networking infrastructure or content scale.

## Locked visual direction

- Technically 3D, presented like premium modern pixel art.
- Fixed near-isometric camera with an orthographic projection.
- Chunky low-poly, voxel-inspired geometry; this is not a destructible voxel world.
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

