#ifndef RENDER_SHADER_UTILS_H
#define RENDER_SHADER_UTILS_H

enum class VertexShaderType
{
	Voxel,
	SwingingDoor,
	SlidingDoor,
	RaisingDoor,
	SplittingDoor
};

enum class PixelShaderType
{
	Opaque,
	OpaqueWithAlphaTestLayer,
	AlphaTested,
	AlphaTestedWithVariableTexCoordUMin // Sliding doors.
};

enum class TextureSamplingType
{
	Default,
	ScreenSpaceRepeatY // Chasms.
};

#endif
