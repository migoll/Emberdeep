import unreal


ASSET_PATH = "/Game/Emberdeep/Materials"
ASSET_NAME = "M_VoxelSurface"


def delete_if_present(object_path: str) -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        unreal.EditorAssetLibrary.delete_asset(object_path)


def create_voxel_surface() -> None:
    object_path = f"{ASSET_PATH}/{ASSET_NAME}"
    delete_if_present(object_path)

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME,
        ASSET_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )
    if not material:
        raise RuntimeError("Could not create M_VoxelSurface")

    material.set_editor_property("two_sided", False)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    # Every authored world/character batch is an ISM. Persist this usage flag so
    # packaged builds never substitute Unreal's glossy default checker material.
    material.set_editor_property("used_with_instanced_static_meshes", True)

    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -520, -120
    )
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.18, 0.18, 0.18, 1.0))

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 80
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.92)

    specular = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 180
    )
    specular.set_editor_property("parameter_name", "Specular")
    specular.set_editor_property("default_value", 0.08)

    emissive_strength = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -520, 300
    )
    emissive_strength.set_editor_property("parameter_name", "EmissiveStrength")
    emissive_strength.set_editor_property("default_value", 0.0)

    emissive = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -220, 240
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", emissive, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(emissive_strength, "", emissive, "B")

    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)
    unreal.MaterialEditingLibrary.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log("EMBERDEEP_VISUAL Created M_VoxelSurface")


create_voxel_surface()
