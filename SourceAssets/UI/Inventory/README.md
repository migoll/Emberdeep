# Inventory UI source assets

Original Emberdeep gothic ARPG interface artwork. Runtime copies are imported to
`/Game/Emberdeep/UI` by `Tools/import_inventory_ui.py`.

- `T_InventoryPanel.png`: equipment mannequin, gold recess, and 5x3 pack grid.
- `T_LootPanel.png`: five-item corpse/chest loot window with two footer buttons.
- `T_ItemIconAtlas.png`: 4x3 item icon atlas used by both windows.

Text and live values are deliberately rendered by UMG rather than baked into the
art. Both windows use a `ScaleBox` with `DownOnly` scaling so they remain fully
visible in a 1280x720 development window without growing excessively at 1440p.
