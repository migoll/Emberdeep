# Asset pipeline

## Source and runtime separation

- Editable source files: `SourceAssets/Characters`, `Environment`, `Animations`, `Textures`, and `Audio`.
- Unreal-imported runtime content: corresponding folders under `Content/Emberdeep`.
- Never edit generated files in `Intermediate`, `DerivedDataCache`, or `Saved`.

## 3D conventions

- Author in Blender using metric units.
- Unreal uses centimeters: 1 Unreal Unit equals 1 centimeter.
- The project-wide fundamental voxel is 4 cm on every axis. Snap solid geometry to that lattice and never non-uniformly scale an individual cell.
- Larger forms use repeated base cells. Animation may translate and rotate rigid cell clusters while preserving cell dimensions.
- Apply transforms before export.
- Place character origins at the feet on the ground plane.
- Use stable skeleton and bone names after animation production begins.
- Export runtime meshes as FBX unless a later test establishes a better project standard.

## Generated voxel environments

- Keep licensed source packs, provenance, and generator instructions together under the relevant `SourceAssets/Environment` folder.
- Prefer deterministic generators and palette-batched instanced geometry for repeated voxel pieces.
- Generators may rasterize conceptual cuboids and imported source modules to the shared 4 cm lattice. They may compress adjacent cells in checked-in data, but runtime rendering must expand them to uniform base-cell instances.
- Use deterministic per-cell palette shades for material texture. Do not rely on stretched photographic or high-frequency PBR textures to define voxel surfaces.
- Keep decorative geometry non-colliding. Author a small number of simple gameplay collision proxies separately.
- Commit generated previews and runtime data so teammates do not need the original DCC tool to build or review the project.

## Version control

- Commit `.blend` source and exported runtime assets through Git LFS.
- Keep file names lowercase with underscores.
- Do not overwrite another contributor's asset silently.
- Every contributed asset needs a short README entry describing author, source file, export settings, and license.
