# Emberdeep HUD contract

The combat HUD is one bottom-center connected assembly. Health and the class
resource stay attached to the action bar at every supported aspect ratio.

The long recessed channel below the four ability sockets and potion socket is
permanently reserved for experience progress. It uses an amber/gold runtime
fill driven by `AEmberdeepCharacter::GetExperienceNormalized()`. Do not remove,
repurpose, or visually detach this XP track when revising the HUD.

Runtime layer order inside the assembly is:

1. authored connected frame;
2. health, class-resource, ability, potion, and XP runtime fills inside their
   measured apertures;
3. cooldown overlays, hotkeys, potion count, level, and other text.

The source frame is `SourceAssets/UI/HUD/T_ConnectedCombatHUD_XP.png`. Unreal
imports it as `/Game/Emberdeep/UI/T_ConnectedCombatHUD` with UI compression and
no mipmaps.
