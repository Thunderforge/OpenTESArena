#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "ChunkManager.h"
#include "../Collision/CollisionChunkManager.h"
#include "../Entities/EntityChunkManager.h"
#include "../Rendering/RenderChunkManager.h"
#include "../Rendering/RenderSkyManager.h"
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
	RenderSkyManager renderSkyManager;
	//ParticleManager; // @todo
	//RenderParticleManager; // @todo

	ScopedObjectTextureRef gameWorldPaletteTextureRef, lightTableTextureRef;

	SceneManager();

	void cleanUp();
};

struct SceneTransitionState
{
	MapType mapType;
	int levelIndex;
	int skyIndex;
};

struct SceneInteriorSavedState
{
	// Don't need to store levelIndex or skyIndex since the LEVELUP/DOWN voxel lets us infer it
};

struct SceneCitySavedState
{
	int weatherIndex;
	CoordInt3 returnCoord;
};

using SceneWildSavedState = SceneCitySavedState;

#endif