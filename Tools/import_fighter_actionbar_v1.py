import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
DESTINATION = "/Game/Emberdeep/UI"


def import_ui_texture(source_parts: tuple[str, ...], asset_name: str) -> None:
    source_file = os.path.join(PROJECT_DIR, "SourceAssets", "UI", *source_parts)
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
        raise RuntimeError(f"Fighter action-bar import failed: {source_file}")
    texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property(
        "compression_settings",
        unreal.TextureCompressionSettings.TC_EDITOR_ICON,
    )
    texture.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    unreal.log(f"EMBERDEEP_ASSET Imported {asset_name}")


import_ui_texture(("HUD", "ActionBar", "T_ActionBarFrame_v1.png"), "T_ActionBarFrame_v1")
import_ui_texture(("Icons", "Fighter", "T_FighterAbility_01_Charge.png"), "T_FighterAbility_01_Charge")
import_ui_texture(("Icons", "Fighter", "T_FighterAbility_02_AxeStrike.png"), "T_FighterAbility_02_AxeStrike")
import_ui_texture(("Icons", "Fighter", "T_FighterAbility_03_ShieldSlam.png"), "T_FighterAbility_03_ShieldSlam")
import_ui_texture(("Icons", "Fighter", "T_FighterAbility_04_Whirlwind.png"), "T_FighterAbility_04_Whirlwind")
