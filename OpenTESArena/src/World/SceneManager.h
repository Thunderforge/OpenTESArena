#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "ChunkManager.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Rendering/RenderChunkManager.h"
#include "../Rendering/RenderTextureUtils.h"
#include "../Sky/SkyInstance.h"
#include "../Voxels/VoxelChunkManager.h"

struct SceneManager
{
	// Chunk managers for the active scene.
	ChunkManager chunkManager;
	VoxelChunkManager voxelChunkManager;
	EntityChunkManager entityChunkManager;
	CollisionChunkManager collisionChunkManager;
	RenderChunkManager renderChunkManager;

	// Game world systems not tied to chunks.
	//SkyInstance skyInstance; // @todo
	//RenderSkyManager; // @todo
	//ParticleManager; // @todo
	//RenderParticleManager; // @todo

	int mapDefIndex; // Points into map definitions list (there might be an exterior and interior in memory).
	int activeLevelIndex; // For indexing into map definition.
	int activeSkyIndex;
	//int activeWeatherIndex; // @todo

	ScopedObjectTextureRef gameWorldPaletteTextureRef, lightTableTextureRef;
	double ceilingScale;

	SceneManager();

	void init(int mapDefIndex);

	void setLevelActive(int index);

	void cleanUp();
};

struct SceneTransitionLocationIDs
{
	int provinceID;
	int locationID;
};

struct SceneTransitionState
{
	int mapDefIndex;
	int levelIndex;
	int skyIndex;
	std::optional<SceneTransitionLocationIDs> locationIDs;
	// @todo
};

#endif
