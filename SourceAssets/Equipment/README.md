# Separate voxel equipment

Every runtime weapon is an independent Blockbench Generic Model. The fighter
rig only animates the generic `MainHand` and `OffHand` anchors.

Current weapon mapping:

- `NotchedIronAxe` -> `Weapons/notched_iron_axe.bbmodel`
- `BonecarverAxe` -> `Weapons/bonecarver_axe.bbmodel`
- `WardenCleaver` -> `Weapons/warden_cleaver.bbmodel`
- `EmberdeepOath` -> `Weapons/emberdeep_oath.bbmodel`
- Default off-hand -> `OffHands/fighter_shield_v0.bbmodel`

Double-click `OPEN_EQUIPMENT_IN_BLOCKBENCH.cmd` to open the editable assets.
Keep the root group named `Equipment`, keep cubes on whole grid units, and keep
palette-prefixed cube names such as `Steel__001`. The origin is the grip point:
do not move the complete Equipment group to place it on the character. Placement
and animation come from the fighter's MainHand/OffHand anchors.

After saving, close Unreal and run the fighter's `SYNC_TO_GAME.cmd`. That one
sync command imports the character geometry, animations, and every equipment
asset before rebuilding the editor target.
