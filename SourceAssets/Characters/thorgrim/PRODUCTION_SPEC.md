# Thorgrim production specification

## Silhouette and scale

Thorgrim is a squat, barrel-shaped beast-slayer approximately four-and-a-quarter heads tall. His silhouette is read in this order:

1. Mammoth fur and paired shoulder tusks.
2. Round tusk shield and heavy bone-and-steel axe.
3. Swept hair, large segmented beard, and unhelmeted face.
4. Skull buckle, pouches, straps, and minor trophies.

The v0 visual is 165 cm tall, with a core shoulder width near 85 cm and a fur-and-tusk silhouette approaching 115 cm. His collision capsule remains narrower than his visible equipment.

## Voxel hierarchy

- Fundamental cell: 4 x 4 x 4 cm for every solid part.
- 16-24 cm arrangements: torso, limbs, boots, shield mass, and major armour volumes.
- 8-16 cm arrangements: fur, beard, hair, plates, tusks, and axe forms.
- Single-cell accents: shield rim, belt skull, facial planes, and fasteners.
- Details below 4 cm are reserved for non-solid effects; they are not modeled as geometry.

Curves are stepped constructions. Do not replace the tusks, shield, hair, or fur with smooth geometry carrying a pixel texture.

## Palette

| Role | Base colour |
|---|---|
| Night undercloth | `#18202B` |
| Steel | `#555A60` |
| Ash detail | `#777572` |
| Bone and tusk | `#D2C2A2` |
| Hide and leather | `#8A5F3C` |
| Fur and hair | `#49372D` |
| Skin | `#B87454` |
| Wood | `#4E3525` |
| Cold cloth accent | `#243442` |

Gold and emissive effects are intentionally absent from the base character so later rewards retain their visual value.

## Skeletal conversion modules

- `body_base`
- `hair_beard`
- `underlayer`
- `mammoth_mantle_tusks`
- `bracers_gloves`
- `waist_trophies`
- `boots`
- `bone_axe`
- `tusk_shield`

The v0 wearable parts may be combined into one skeletal body, but the axe and shield remain separate socketed meshes.

## Animation priorities

The v0 runtime proof currently covers guard idle, heavy locomotion, and the axe-cleave timing with six procedural rigid voxel zones. It is a readability and timing prototype; the remaining actions and clean anatomical grouping belong to the authored skeletal conversion.

1. Guard idle.
2. Heavy run.
3. Outside-in axe cleave.
4. Shield slam.
5. Short shoulder-dash dodge rather than an acrobatic roll.
6. Heavy hit reaction.
7. Kneeling collapse death.

Use in-place movement, rigid cluster weighting, and approximately 10-12 held visual poses per second. The axe head, shield rim, and paired shoulder tusks must remain separated from the torso in all eight gameplay-facing directions.
