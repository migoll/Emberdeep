import os

import unreal


project_dir = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
source_dir = os.path.join(project_dir, "SourceAssets", "Audio", "generated")
destination = "/Game/Emberdeep/Audio"

names = (
    "S_FighterSwingLight",
    "S_FighterSwingHeavy",
    "S_BoneHit",
    "S_HeavyImpact",
    "S_Dodge",
    "S_EnemyWindup",
    "S_PlayerHurt",
    "S_EnemyDeath",
)

tasks = []
for name in names:
    task = unreal.AssetImportTask()
    task.filename = os.path.join(source_dir, f"{name}.wav")
    task.destination_path = destination
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for name in names:
    asset = unreal.load_asset(f"{destination}/{name}")
    if asset is None:
        raise RuntimeError(f"Combat audio import failed: {name}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset)

unreal.log(f"EMBERDEEP_ASSET Imported {len(names)} deterministic combat sounds")
