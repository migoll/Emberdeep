# Asset pipeline

## Source and runtime separation

- Editable source files: `SourceAssets/Characters`, `Environment`, `Animations`, `Textures`, and `Audio`.
- Unreal-imported runtime content: corresponding folders under `Content/Emberdeep`.
- Never edit generated files in `Intermediate`, `DerivedDataCache`, or `Saved`.

## 3D conventions

- Author in Blender using metric units.
- Unreal uses centimeters: 1 Unreal Unit equals 1 centimeter.
- Apply transforms before export.
- Place character origins at the feet on the ground plane.
- Use stable skeleton and bone names after animation production begins.
- Export runtime meshes as FBX unless a later test establishes a better project standard.

## Version control

- Commit `.blend` source and exported runtime assets through Git LFS.
- Keep file names lowercase with underscores.
- Do not overwrite another contributor's asset silently.
- Every contributed asset needs a short README entry describing author, source file, export settings, and license.

