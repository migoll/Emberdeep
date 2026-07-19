# Low-density fighter v0

This is Emberdeep's deliberately simpler character-density comparison. It takes
its target from the approved gameplay combat screenshot rather than Thorgrim's
close presentation sheet.

- Fundamental runtime lattice: 4 cm, unchanged across the project.
- Visual authoring module: mostly 2x2 cells (8 cm), with isolated 4 cm cells only
  for the eyes, sword tip, and stepped shield edge.
- Silhouette priorities: oversized helmet/head, shoulder armour, hands, sword,
  shield, and broad value separation.
- Detail intentionally omitted: layered fur locks, individual straps, trophies,
  rivets, and dense lamellar armour.

## Visual editing in Blockbench

`fighter_v0.bbmodel` is the editable source of truth. Double-click
`OPEN_IN_BLOCKBENCH.cmd`, edit in Blockbench's **Edit** mode, and use **File >
Save Project**. If Ctrl+S asks for a PNG, the texture is selected; click the 3D
viewport or Outliner first and then save the project.

Editing rules enforced by the converter:

- Keep all cube positions and sizes on whole grid units. One Blockbench unit is
  one 4 cm Emberdeep voxel.
- Keep `Body` plus the generic `MainHand` and `OffHand` anchors. Inside `Body`,
  keep the six rig groups `Torso`, `Head`, `LeftArm`, `RightArm`, `LeftLeg`, and
  `RightLeg`. The legacy name
  The older `Sword`/`Axe` and `Shield` names remain accepted as migration aliases.
- Duplicate existing cubes when adding geometry so names retain their palette
  prefix, such as `Steel__` or `Hide__`.
- Cubes stay axis-aligned. Rotate the complete `MainHand` or `OffHand` group
  instead of rotating individual cubes; arbitrary equipment-group rotations are
  converted to Unreal quaternions without Euler-angle loss.
- Large Blockbench cuboids are an editing convenience only. The converter
  expands them back into identical 4 cm runtime cells.

For the normal checked-in workflow, close Unreal and double-click
`SYNC_TO_GAME.cmd`. It validates the grid, regenerates the preview, voxel data,
and animation data, and builds the Unreal editor target.

For live editing, double-click `LIVE_SYNC_TO_GAME.cmd` once. Leave its
PowerShell window open, edit either **Edit** or **Animate** in Blockbench, and
save the `.bbmodel` project with Ctrl+S. The open game updates in roughly a
quarter of a second; no Unreal rebuild or manual sync is needed. Live Sync also
watches all weapon and shield `.bbmodel` files under `SourceAssets/Equipment`.

Live Sync is a preview loop. Before committing or sharing finished changes, run
`SYNC_TO_GAME.cmd` once so the generated C++ runtime data is updated too.

`MainHand` and `OffHand` contain preview geometry so attacks remain readable in
Blockbench. Runtime equipment comes from the independent models under
`SourceAssets/Equipment`; equipping another weapon swaps that model while the
same anchor animation continues to drive it.

## Animation editing in Blockbench

Open the **Animate** workspace at the top-right. The timeline's animation menu
contains `Idle`, `Walk`, `BasicAttack`, `HeavyAttack`, and `Dodge`. Select a rig
group in the Outliner, move the playhead, then use the rotation or position
keyframe buttons. Play the clip with the timeline play button.

The converter samples these clips at 12 poses per second when
`SYNC_TO_GAME.cmd` runs. It currently accepts ordinary numeric position and
rotation keyframes with linear or step interpolation. The clip names and rig
group names are the contract between Blockbench and Unreal.

## Command line

Run from the repository root:

```powershell
python SourceAssets/Characters/Fighter/generate_fighter.py
python SourceAssets/Characters/Fighter/fighter_animation_bridge.py
python SourceAssets/Characters/Fighter/generate_fighter.py --check
python SourceAssets/Characters/Fighter/fighter_animation_bridge.py --check
```

The generator reads `fighter_v0.bbmodel` and writes `fighter_v0_preview.png`
plus the checked-in Unreal runtime data at
`Source/Emberdeep/Private/Gameplay/FighterVoxelData.inl`.

`--bootstrap-blockbench` creates the initial editable master. It refuses to
overwrite an existing file. `--force-bootstrap-blockbench` is a destructive
reset and discards all manual Blockbench edits.
