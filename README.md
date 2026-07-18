# Emberdeep

Emberdeep is a dark-fantasy cooperative action RPG built with Unreal Engine 5.8. Its visual direction combines chunky low-poly, voxel-inspired 3D geometry with a deliberately pixelated presentation.

The core loop is:

> Kill -> loot -> grow stronger -> descend deeper.

## Current phase

Phase 0B is active. The current build proves authoritative direct-IP co-op for
two to five players in the Broken Caravan Camp encounter. It does not include
accounts, matchmaking, relay hosting, persistence, or public servers.

The current slice contains:

- a generated Broken Caravan Camp arena assembled from a reusable voxel kit;
- fixed orthographic near-isometric camera;
- playable code-instanced Thorgrim visual with a chunky voxel-built silhouette;
- screen-relative WASD movement and invulnerable dodge;
- basic and heavy melee attacks with cooldowns and knockback;
- two skeletons and an elite Bone Warden with proximity aggro and a telegraphed slam;
- health, death, automatic encounter restart, enemy hit flashes, and defeat feedback;
- dropped gold pickups and victory tracking;
- replicated level and experience rewards;
- a dark-fantasy HUD with dynamic party roster, minimap/objective panel, enemy health,
  health/dodge orbs, action bar, cooldown state, experience, and gold;
- Host/Join menu with a direct `IP[:port]` field and a five-player cap;
- server-authoritative attacks, dodge, health, enemies, loot, encounter state,
  checkpoint respawn, and disconnect cleanup;
- verified two-client joins, replicated encounter state, repeated independent
  respawns, and party cleanup after disconnect.

## Controls

- `WASD`: move
- Left mouse or `Space`: basic attack hook
- Right mouse or `E`: heavy attack hook
- Left Shift: dodge hook
- Mouse wheel: smooth camera zoom
- `Escape`: quit standalone test

## Multiplayer test

1. Launch the game and choose **Host Game**.
2. Launch a second session, keep `127.0.0.1:7777` for the same PC, and choose
   **Join Game**.
3. For LAN, enter the host computer's local IPv4 address.
4. For direct internet IP, the host must forward UDP port `7777` and allow the
   game through Windows Firewall.

The listen server supports at most five connected players. There is no
matchmaking or relay fallback in Phase 0B.

Developers can run the repeatable headless verification with:

```powershell
.\Tools\Test-Multiplayer.ps1
```

Designer handoff templates live in `SourceAssets/Characters/Fighter` and
`SourceAssets/Environment/Campground`. Runtime imports belong under the matching
`Content/Emberdeep` folders.

Thorgrim's generated source model, portable exports, concept, and production notes
live in `SourceAssets/Characters/thorgrim`.

The camp concept, CC0 source kit, deterministic generator, preview, and provenance
live in `SourceAssets/Environment/Campground`.

## Setup

1. Install Unreal Engine 5.8.
2. Install MSVC v143 x64 build tools and a Windows 11 SDK.
3. Double-click `Emberdeep.uproject` after the first successful C++ build.

Generated folders such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` are intentionally ignored.

See `ROADMAP.md`, `BACKLOG.md`, `docs/ART_DIRECTION.md`, and `docs/ASSET_PIPELINE.md` before adding systems or content.
