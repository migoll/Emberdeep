import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DESTINATION = "/Game/Emberdeep/UI"


def import_ui_texture(source_name: str, asset_name: str) -> None:
    source_file = os.path.join(
        PROJECT_DIR,
        "SourceAssets",
        "UI",
        "HUD",
        source_name,
    )

    task = unreal.AssetImportTask()
    task.filename = source_file
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if texture is None:
        raise RuntimeError(f"Target-matched HUD import failed: {source_file}")

    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property(
        "mip_gen_settings",
        unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
    )
    texture.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_EDITOR_ICON,
    )
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log(f"EMBERDEEP_ASSET Imported {asset_name}")


import_ui_texture("T_CombatHUD_TargetMatched.png", "T_CombatHUD_TargetMatched")
import_ui_texture("T_PartyRow_TargetMatched.png", "T_PartyRow_TargetMatched")
import_ui_texture("T_MinimapFrame_TargetMatched.png", "T_MinimapFrame_TargetMatched")
