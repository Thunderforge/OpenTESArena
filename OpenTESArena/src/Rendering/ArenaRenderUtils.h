#ifndef ARENA_RENDER_UTILS_H
#define ARENA_RENDER_UTILS_H

#include <cstdint>
#include <string>

namespace ArenaRenderUtils
{
	constexpr int SCREEN_WIDTH = 320;
	constexpr int SCREEN_HEIGHT = 200;
	constexpr double ASPECT_RATIO = static_cast<double>(SCREEN_WIDTH) / static_cast<double>(SCREEN_HEIGHT);
	constexpr int BITS_PER_PIXEL = 8;

	constexpr int FRAMES_PER_SECOND = 25;

	// Texture limits.
	constexpr int DEFAULT_VOXEL_TEXTURE_COUNT = 64;
	constexpr int DEFAULT_FLAT_TEXTURE_COUNT = 256;

	// Height ratio between normal pixels and tall pixels.
	constexpr double TALL_PIXEL_RATIO = 1.20;

	// Amount of a sliding/raising door that is visible when fully open.
	constexpr double DOOR_MIN_VISIBLE = 0.10;

	// Hardcoded palette indices with special behavior in the original game's renderer.
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_LOWEST = 1;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_HIGHEST = 13;
	constexpr uint8_t PALETTE_INDEX_LIGHT_LEVEL_DIVISOR = 14;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_LOWEST = 1;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_HIGHEST = 13;
	constexpr uint8_t PALETTE_INDEX_SKY_LEVEL_DIVISOR = 14;
	constexpr uint8_t PALETTE_INDEX_RED_SRC1 = 14;
	constexpr uint8_t PALETTE_INDEX_RED_SRC2 = 15;
	constexpr uint8_t PALETTE_INDEX_RED_DST1 = 158;
	constexpr uint8_t PALETTE_INDEX_RED_DST2 = 159;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT = 113;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT_ACTIVE = 97;
	constexpr uint8_t PALETTE_INDEX_NIGHT_LIGHT_INACTIVE = 112;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_EVEN_ROW = 30;
	constexpr uint8_t PALETTE_INDEX_PUDDLE_ODD_ROW = 103;
	constexpr uint8_t PALETTE_INDEX_DRY_CHASM_COLOR = 112; // @todo: might not be correct, need to check with light tables
	constexpr uint8_t PALETTE_INDEX_RAINDROP = 103;
	constexpr uint8_t PALETTE_INDEX_SNOWFLAKE = 16;

	constexpr double PLAYER_LIGHT_RADIUS = 5.0; // @todo: this might be a little too high.
	constexpr double STREETLIGHT_LIGHT_RADIUS = 4.0; // @todo: this might be a little too high.

	const std::string CHASM_WATER_FILENAME = "WATERANI.RCI";
	const std::string CHASM_LAVA_FILENAME = "LAVAANI.RCI";

	// Various functions for determining the type of palette index.
	bool IsGhostTexel(uint8_t texel);
	bool IsPuddleTexel(uint8_t texel);
	bool IsCloudTexel(uint8_t texel);
}

#endif
