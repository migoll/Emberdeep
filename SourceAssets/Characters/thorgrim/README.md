# Thorgrim v0 character asset

## Status

Thorgrim is the first playable visual proof for Emberdeep's agreed character density. The v0 model is a code-instanced voxel character with a separate portable mesh export. It proves silhouette, scale, palette, equipment readability, lighting, and stepped rigid-cluster animation in the current prototype; an authored skeletal animation library remains a later conversion step.

## Design authority

- Primary concept: `thorgrim_concept_primary.png`
- Overall moodboard: `../../References/moodboard_visual_direction.png`
- Written direction: `../../../docs/ART_DIRECTION.md`

The concept is interpreted rather than traced literally. Every visible solid uses the project-wide 4 cm cell; large and medium forms are arrangements of those cells rather than differently sized cubes.

## Files

- `generate_thorgrim.py`: editable procedural source of truth.
- `thorgrim_v0_preview.png`: deterministic isometric preview.
- `exports/thorgrim_v0.obj`: meter-scale, Z-up fixed-grid interchange mesh.
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

The checked-in GLB predates the fixed-grid conversion and remains a legacy preview until the offline exporter is rerun. The current OBJ and Unreal data are authoritative.

Legacy GLB output:

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
- Fundamental voxel: 4 cm, shared with all Emberdeep solid art.
- Runtime representation: 8,746 visible fixed-size cube instances split across body, axe, and shield parts with nine palette families and three deterministic value shades.
- Mesh and equipment collision: disabled; the character capsule remains authoritative.

The runtime representation deliberately avoids one component per voxel. Part, palette, and shade batching creates 81 instanced components. Keeping the axe and shield on separate 4 cm-snapped pivots preserves attack feedback now and leaves a clean path to replace either item later.

## Runtime animation proof

The current Unreal prototype classifies the body voxels into six rigid spatial zones: torso, head, left/right arms, and left/right legs. It evaluates a new visual pose at 12 frames per second while gameplay movement and collision remain continuous.

- Idle: restrained breathing, weight shift, and head motion.
- Locomotion: alternating rigid arm and leg swings inferred from velocity, including replicated remote movement.
- Attack: body commitment, right-arm axe swing, and shield brace driven through the existing replicated attack visual.
- Voxel integrity: every cell retains its original 4 cm scale; animation only translates or rotates rigid groups.

This is a lightweight in-engine proof, not the final production rig. The spatial grouping is intentionally coarse and may include cloak or armour cells near a limb. It exists so animation timing and readability can be tested in-game before the one-time authored rig conversion.

## Future skeletal conversion

Import the OBJ into Blender, preserve the block silhouette, remove hidden and coplanar faces, and split the axe and shield from the body. Build a compact humanoid rig with rigid weights, export FBX 2020.2, and retain the current runtime construction as a fallback until the skeletal asset is validated in Unreal. Once that Skeletal Mesh exists, external humanoid clips such as the CC0 Quaternius animation libraries can be retargeted with Unreal's IK Retargeter and then reduced to the project's stepped visual timing.

Suggested runtime names:

- `SK_Thorgrim`
- `SM_Thorgrim_BoneAxe`
- `SM_Thorgrim_TuskShield`

## Provenance and license

The primary image is a project-owner-provided AI concept. The procedural model and generated exports are original Emberdeep project work created from that direction without third-party source assets. They are project-internal assets; the project owner must confirm final distribution licensing for the supplied concept image.
