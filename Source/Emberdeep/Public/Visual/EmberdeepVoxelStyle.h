#pragma once

#include "CoreMinimal.h"

namespace EmberdeepVoxelStyle
{
	inline constexpr float UnitCm = 4.0f;
	inline constexpr float RenderFill = 0.94f;
	inline constexpr int32 ShadeCount = 3;

	inline FVector CellCenter(int32 X, int32 Y, int32 Z)
	{
		return FVector(
			(static_cast<float>(X) + 0.5f) * UnitCm,
			(static_cast<float>(Y) + 0.5f) * UnitCm,
			(static_cast<float>(Z) + 0.5f) * UnitCm);
	}

	inline FVector InstanceScale()
	{
		const float UniformScale = UnitCm * RenderFill / 100.0f;
		return FVector(UniformScale);
	}

	inline int32 SelectShade(int32 X, int32 Y, int32 Z, int32 PaletteSeed)
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(X)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Y)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Z)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(PaletteSeed)) * 16777619u;
		const uint32 Selector = Hash % 16u;
		if (Selector < 3u)
		{
			return 0;
		}
		return Selector == 15u ? 2 : 1;
	}

	inline FLinearColor ShadeColor(const FLinearColor& BaseColor, int32 ShadeIndex)
	{
		const float Factor = ShadeIndex == 0 ? 0.72f : (ShadeIndex == 2 ? 1.16f : 1.0f);
		return FLinearColor(
			FMath::Clamp(BaseColor.R * Factor, 0.0f, 1.0f),
			FMath::Clamp(BaseColor.G * Factor, 0.0f, 1.0f),
			FMath::Clamp(BaseColor.B * Factor, 0.0f, 1.0f),
			BaseColor.A);
	}
}
