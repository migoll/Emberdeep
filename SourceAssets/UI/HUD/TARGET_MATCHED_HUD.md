# Target-matched combat HUD

These three runtime UI textures were generated specifically for Emberdeep from the approved
1536x1024 dungeon screenshot, then keyed to RGBA, cropped and imported without mipmaps:

- `T_CombatHUD_TargetMatched.png` (2157x389)
- `T_PartyRow_TargetMatched.png` (1831x495)
- `T_MinimapFrame_TargetMatched.png` (1227x1224)

The Unreal import entry point is `Tools/import_target_matched_hud.py`. The deterministic chroma
cleanup helper is `Tools/Remove-HudChroma.ps1`.

## Combat frame runtime apertures

Coordinates use the 2157x389 source canvas:

- health orb: center `(267, 176)`, radius `124`
- ability slots: `(560,132,202,156)`, `(817,132,188,156)`,
  `(1057,133,189,155)`, `(1297,132,187,156)`
- potion slot: `(1536,133,72,155)`
- resource orb: center `(1888,176)`, radius `124`
- XP fill: `(509,344,1138,14)`

All live fills, icons and cooldown masks render first. The single connected frame renders last.

## Generation method

The built-in image generation tool used the approved dungeon screenshot as a style/composition
reference. Each prompt requested one isolated Unreal HUD texture on uniform `#00FF00`, with live
apertures also keyed green. The cleanup pass removes green by dominance, despills edge pixels,
crops to visible alpha and saves 32-bit RGBA PNGs. No full-screen concept image is involved.

Core combat-frame prompt:

> Create one connected compact dark-fantasy combat HUD frame matching the reference's proportions
> and restraint: a circular health frame attached left, exactly four equal ability sockets, one
> narrower potion socket, a circular resource frame attached right, and a thin recessed XP track.
> Use worn black iron, aged bronze and restrained gothic ARPG detailing. All live apertures and the
> background are uniform #00FF00. No text, icons, liquids, watermark, detached parts, bright gold,
> glossy mobile styling or extra sockets.
