import os
import unreal


PROJECT_DIR = unreal.Paths.project_dir()
SOURCE_DIR = os.path.join(PROJECT_DIR, "SourceAssets", "UI", "Inventory")
DESTINATION = "/Game/Emberdeep/UI"


def import_texture(filename: str, asset_name: str) -> None:
    source = os.path.join(SOURCE_DIR, filename)
    if not os.path.isfile(source):
        raise RuntimeError(f"Missing UI source asset: {source}")

    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if not texture:
        raise RuntimeError(f"Failed to import {asset_name}")
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log(f"EMBERDEEP_UI_IMPORTED {asset_name}")


import_texture("T_InventoryPanel.png", "T_InventoryPanel")
import_texture("T_LootPanel.png", "T_LootPanel")
import_texture("T_ItemIconAtlas.png", "T_ItemIconAtlas")
