# Emberdeep

Emberdeep is a dark-fantasy cooperative action RPG built with Unreal Engine 5.8. Its visual direction combines chunky low-poly, voxel-inspired 3D geometry with a deliberately pixelated presentation.

The core loop is:

> Kill -> loot -> grow stronger -> descend deeper.

## Current phase

Phase 0B remains active, with a deliberately bounded Phase 2 progression slice
pulled forward for playtesting. The current build proves authoritative direct-IP
co-op for two to five players across a short Camp -> Ashen Crypt -> Cinder Vault
run. Progress survives death and room transitions for the hosted run, but is not
saved to disk. There are no accounts, matchmaking, relay hosts, or public servers.

The current slice contains:

- a peaceful generated Broken Caravan Camp assembled from a reusable voxel kit;
- an Ashen Crypt combat room and a visually distinct Cinder Vault reward room;
- party-wide portals that move every connected player between run stages;
- fixed orthographic near-isometric camera;
- playable code-instanced Thorgrim visual with a chunky voxel-built silhouette;
- screen-relative WASD movement and invulnerable dodge;
- basic and heavy melee attacks with cooldowns and knockback;
- two skeletons and a tier-scaled elite Bone Warden with proximity aggro and a telegraphed slam;
- health, death, automatic encounter restart, enemy hit flashes, and defeat feedback;
- dropped gold plus server-owned enemy, cache, and reward-chest loot;
- `F` interaction prompts and a shared WoW-style clickable loot window;
- a twelve-slot `I` inventory with weapon, armor, and trinket equipment;
- equipped damage, maximum-health, and armor bonuses that affect real combat;
- a guaranteed legendary reward at the end of each run and increasing run tiers;
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
- `F`: use a nearby portal, corpse drop, cache, or reward chest
- `I`: open inventory and click an item to equip it
- `Escape`: quit standalone test

## Progression run test

1. Host the game and walk to the purple camp portal.
2. Press `F`; the whole party enters the Ashen Crypt and the encounter begins.
3. Kill a skeleton, walk to its drop, press `F`, then click an item to take it.
4. Press `I` and click the item to equip it. The HUD damage/armor values update.
5. Open the Crypt Cache, clear the Bone Warden, and use the unlocked vault portal.
6. Open the Cinder Vault reliquary for one guaranteed legendary item.
7. Return to camp. The next descent is a higher tier, and inventory, equipment,
   gold, level, and experience remain for the hosted run.

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
