# Fighter action bar v1

`T_ActionBarFrame_v1.png` is the RGBA source used by Emberdeep's runtime HUD.
Its visible alpha crop is `(7,187)-(1908,623)` in the 1912x823 source. The two
orbs and all eight ability sockets are genuinely transparent.

Runtime slot mapping currently uses:

- LMB: Axe Strike
- RMB: Shield Slam
- Shift: Charge
- Q: Whirlwind
- Slots 5-8: reserved

Run `Tools/import_fighter_actionbar_v1.py` through Unreal Editor Python after
replacing the frame or any source icon. Textures import without mipmaps using UI
compression. The `_Chroma` image is retained only as an editable intermediate.
