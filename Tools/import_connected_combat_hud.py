import os

import unreal


project_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source_file = os.path.join(
    project_dir,
    "SourceAssets",
    "UI",
    "HUD",
    "T_ConnectedCombatHUD.png",
)

task = unreal.AssetImportTask()
task.filename = source_file
task.destination_path = "/Game/Emberdeep/UI"
task.destination_name = "T_ConnectedCombatHUD"
task.automated = True
task.replace_existing = True
task.save = True

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset("/Game/Emberdeep/UI/T_ConnectedCombatHUD")
if texture is None:
    raise RuntimeError(f"Connected combat HUD import failed: {source_file}")

texture.set_editor_property("filter", unreal.TextureFilter.TF_BILINEAR)
texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property("srgb", True)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log("EMBERDEEP_ASSET Connected combat HUD imported")
