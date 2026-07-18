# Thorgrim v0 character asset

## Status

Thorgrim is the first playable visual proof for Emberdeep's agreed character density. The v0 model is a static, code-instanced voxel character with a separate portable mesh export. It proves silhouette, scale, palette, equipment readability, and lighting in the current prototype; skeletal deformation and authored animation are a later conversion step.

## Design authority

- Primary concept: `thorgrim_concept_primary.png`
- Overall moodboard: `../../References/moodboard_visual_direction.png`
- Written direction: `../../../docs/ART_DIRECTION.md`

The concept is interpreted rather than traced literally. Large clusters define the body and equipment, medium clusters define fur, armour, hair, and tusks, and very small blocks are restricted to the face and belt trophies.

## Files

- `generate_thorgrim.py`: editable procedural source of truth.
- `thorgrim_v0_preview.png`: deterministic isometric preview.
- `exports/thorgrim_v0.obj`: meter-scale, Z-up interchange mesh with named block objects.
- `exports/thorgrim_v0.mtl`: flat nine-colour material palette.
- `exports/thorgrim_v0.glb`: static glTF 2.0 production candidate with embedded geometry and materials.
- `godot/export_thorgrim.ps1`: one-command offline OBJ/MTL-to-GLB export and validation.
- `godot/export_thorgrim.gd`: deterministic mesh conversion and Godot glTF export logic.
- `godot/validate_glb.ps1`: dependency-free GLB container and reference validator.
- `../../../Source/Emberdeep/Private/Gameplay/ThorgrimVoxelData.inl`: generated Unreal runtime transforms.

Run `python SourceAssets/Characters/thorgrim/generate_thorgrim.py` from the repository root after changing the source. The generator requires NumPy and Pillow and writes all derived files deterministically.

## Offline GLB export

After regenerating the OBJ and MTL, run this from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File SourceAssets/Characters/thorgrim/godot/export_thorgrim.ps1
```

The wrapper accepts an optional `-GodotExecutable` path. Otherwise it resolves `godot` or `godot4` from `PATH`, then looks for a portable stable Windows console build under a local user Downloads folder. The headless exporter parses the current OBJ and MTL directly, converts Unreal's X-forward/Z-up authoring basis to the standard glTF basis, creates one indexed mesh with nine flat rough palette surfaces, writes the embedded GLB, and reads it back through Godot before running the structural validator.

Current validated output:

- Source: 213 named block objects, 1,704 vertices, and 1,278 quad faces.
- GLB: glTF 2.0, one scene, two nodes, one static mesh, and nine embedded palette materials.
- Materials: Night, Steel, Ash, Bone, Hide, Fur, Skin, Wood, and Cloth; no texture dependency.
- Bounds: 1.631 m tall, including the current equipment silhouette.
- File size: 268,284 bytes.

## Runtime construction

- Unreal facing direction: `+X`.
- Authoring origin: centred between the feet.
- Target visual height: 165 cm.
- Gameplay capsule: 42 cm radius, 80 cm half-height.
- Runtime representation: 213 cube instances split across body, axe, and shield parts with nine flat palette materials.
- Mesh and equipment collision: disabled; the character capsule remains authoritative.

The runtime representation deliberately avoids one component per voxel. Part-and-palette batching creates 27 components, of which 18 currently contain geometry. Keeping the axe and shield on separate pivots preserves attack feedback now and leaves a clean path to replace either item later.

## Future skeletal conversion

Import the OBJ into Blender, preserve the block silhouette, remove hidden and coplanar faces, and split the axe and shield from the body. Build a compact humanoid rig with rigid weights, export FBX 2020.2, and retain the current runtime construction as a fallback until the skeletal asset is validated in Unreal.

Suggested runtime names:

- `SK_Thorgrim`
- `SM_Thorgrim_BoneAxe`
- `SM_Thorgrim_TuskShield`

## Provenance and license

The primary image is a project-owner-provided AI concept. The procedural model and generated exports are original Emberdeep project work created from that direction without third-party source assets. They are project-internal assets; the project owner must confirm final distribution licensing for the supplied concept image.
