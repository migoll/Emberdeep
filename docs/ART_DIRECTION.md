# Art direction

## Style sentence

Chunky low-poly dark fantasy with voxel-inspired geometry and pixel-art presentation, built in Unreal Engine.

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

- Use broad planes and deliberate block structure, not dense smooth geometry.
- Faces, hands, weapons, shields, and class-defining equipment are exaggerated.
- Materials use restricted palettes and simple value bands.
- Avoid noisy detailed PBR surfaces and generic realistic Marketplace appearances.

## Character voxel density

- Characters should read as deliberately built from voxels, not as conventional polygonal models with a pixelated surface treatment.
- Use large voxel clusters for the body and major equipment, medium clusters for armour, hair, fur, and cloth, and small voxels only for faces, fasteners, and magical accents.
- Preserve enough layered detail for helmets, shoulders, chest pieces, weapons, shields, cloaks, and upgraded gear to change the character visibly.
- Judge detail from the normal gameplay camera. Close-up presentation may be denser, but tiny details that disappear during play should not drive the model.
- Thorgrim is the current density benchmark: `SourceAssets/Characters/thorgrim/thorgrim_concept_primary.png`.

## Environment voxel density

- Build ordinary scenery from reusable modular voxel pieces instead of unique fused meshes.
- Keep ground, walls, and distant dressing coarser than characters. Spend smaller voxels on focal props such as the wagon, fire, shelter, and workstation.
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
