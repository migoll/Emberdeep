# Art direction

## Style sentence

Grid-authentic voxel dark fantasy with pixel-art presentation, built in Unreal Engine.

## Primary reference

- Overall visual moodboard: `SourceAssets/References/moodboard_visual_direction.png`.
- Broken Caravan Camp environment: `SourceAssets/Environment/Campground/broken_caravan_camp_concept.png`.
- Use the moodboard for palette, lighting, atmosphere, silhouettes, UI tone, and gameplay-scale readability. More specific written decisions in this document take priority where a generated reference is inconsistent.

## Camera and presentation

- Orthographic, near-isometric camera.
- Initial target rotation: approximately 45 degrees around the world and 50-60 degrees downward.
- Characters must remain readable at the normal gameplay distance.
- Render gameplay to a deliberately low internal resolution and upscale without smoothing.
- UI is rendered crisply above the low-resolution world presentation.

## Forms and materials

- Every visible solid is authored on the same 4 cm base lattice. Character, equipment, prop, ground, and architecture cells are the same physical size.
- Adjacent solid cells meet face-to-face. Do not introduce decorative air gaps between every cell; at gameplay scale those gaps become a shimmering subpixel pattern.
- Build broad planes and larger masses by repeating base cells. A visually larger block may occupy 2x2x2 cells, but it is never one stretched cube.
- Runtime batching or source-data compression may combine cells internally only when the rendered result is indistinguishable from individual fixed-size cells.
- Rigid voxel clusters may move and rotate during animation. Individual cells never shear or receive non-uniform scale.
- Glows, particles, fog, and gameplay telegraphs are exempt because they are effects rather than solid forms.
- Faces, hands, weapons, shields, and class-defining equipment are exaggerated.
- Materials use restricted palettes and deterministic per-cell value bands. The cell-level variation supplies surface texture without bitmap detail being stretched over arbitrary forms.
- Avoid noisy detailed PBR surfaces and generic realistic Marketplace appearances.

## Character voxel density

- Characters should read as deliberately built from voxels, not as conventional polygonal models with a pixelated surface treatment.
- Use large arrangements of the shared cells for the body and major equipment, medium arrangements for armour, hair, fur, and cloth, and individual cells only where facial or equipment readability needs them.
- Preserve enough layered detail for helmets, shoulders, chest pieces, weapons, shields, cloaks, and upgraded gear to change the character visibly.
- Judge detail from the normal gameplay camera. Close-up presentation may be denser, but tiny details that disappear during play should not drive the model.
- Thorgrim is the current density benchmark: `SourceAssets/Characters/thorgrim/thorgrim_concept_primary.png`.

## Environment voxel density

- Build ordinary scenery from reusable modular voxel-cell arrangements instead of unique arbitrary cuboids.
- Ground, walls, and distant dressing may use simpler patterns and fewer silhouette changes than characters, but they retain the same 4 cm cell size. Spend more cell-level variation on focal props such as the wagon, fire, shelter, and workstation.
- Preserve broad movement lanes and clear character silhouettes; environmental micro-detail that disappears at gameplay scale is not a production target.
- The first playable benchmark is the generated Broken Caravan Camp under `SourceAssets/Environment/Campground`.

## Lighting and effects

- Environment remains dark but navigable.
- Warm torches and cold magic create controlled pools of color.
- Disable motion blur and restrain bloom, exposure shifts, fog, and camera shake.
- Telegraphs remain visible beneath party and enemy effects.
- Particles use square/blocky shapes and limited overdraw.

## Animation

- Gameplay simulation remains responsive.
- Visual animation may be stepped to approximately 10-12 poses per second where it improves the miniature/pixel impression.
- Attacks use short anticipation, fast execution, and readable recovery.
