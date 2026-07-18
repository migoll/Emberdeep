# Emberdeep roadmap

Each phase answers one question. Later features are not scaffolded early.

## Phase 0A - Combat and visual proof

Status: complete enough to support the active networking proof. A multiplayer-safe
combat-feel pass now provides attack warnings, hit/death bursts, floating damage,
directional knockback, dodge trails, and local camera punch without moving damage
authority into cosmetic code.

Question: Is one Fighter immediately enjoyable to control, and can the game convincingly match its chunky pixel-styled 3D target?

Includes one Fighter, one arena, one skeleton system plus elite variant, basic/heavy/dodge actions, health, pickups, victory/death/restart, and combat feedback. Excludes multiplayer and persistence.

Complete when the two-to-five-minute demo is readable, responsive, visually coherent, and free of obvious errors.

## Phase 0B - Multiplayer proof

Status: active. Host/Join direct IP, authoritative actions, replicated
encounter state, server enemies/loot, checkpoint respawn, disconnect cleanup,
and automated two-client smoke testing are implemented. Wider remote repetition
and latency testing remain before this phase is complete.

Question: Can two to five players complete the same authoritative encounter without desynchronization?

Includes listen-server Host/Join, replicated movement/actions/health, server-owned enemies and loot, death/respawn, disconnect cleanup, and multi-client tests. Excludes accounts, matchmaking, public hosting, and persistence.

Complete when five remote clients finish repeated encounters with consistent state.

## Phase 1 - Four-class combat prototype

Question: Do Fighter, Rogue, Wizard, and Cleric have distinct, complementary identities?

Includes four complete kits and readable co-op combat. Excludes progression systems and content scale.

## Phase 2 - Dungeon run and loot progression

Status: a bounded vertical slice was intentionally pulled forward by product
direction while Phase 0B remote repetition remains active. Camp -> dungeon ->
reward-room routing, enemy/cache/chest loot, a twelve-slot run inventory,
equipment bonuses, a guaranteed legendary, and rising run tiers are playable.
State currently persists only through death and room transitions in one hosted
session; content scale, bosses, disk persistence, and a full loot table remain.

Question: Does kill-loot-grow-deeper remain satisfying across a complete short run?

Includes several encounters, boss, lightweight inventory/equipment, reward room, and difficulty tiers.

## Phase 3 - Persistent camp and characters

Question: Does returning from a run create meaningful persistent progress?

Includes accounts only if needed, saved characters, camp, contracts, equipment persistence, and consequences.

## Phase 4 - Online service hardening

Question: Can strangers or a wider friend group connect reliably and safely?

Includes dedicated authoritative servers, deployment, monitoring, matchmaking, authentication, persistence security, and recovery.

## Phase 5 - Shared realm experiment

Question: Does a larger shared zone improve the game enough to justify MMO-level complexity?

Includes one bounded shared realm/social zone with interest management and instanced 4-5 player dungeons. It does not promise one seamless global world.

## Phase 6 - AI Dungeon Master

Question: Can validated between-run generation create memorable contracts and consequences without destabilizing the game?

Includes schema-validated generated contracts, dialogue, lore, and deterministic fallbacks. No LLM is called during real-time combat.
