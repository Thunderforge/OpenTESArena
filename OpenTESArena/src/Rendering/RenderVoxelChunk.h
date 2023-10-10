#ifndef RENDER_VOXEL_CHUNK_H
#define RENDER_VOXEL_CHUNK_H

#include <unordered_map>
#include <vector>

#include "RenderDrawCall.h"
#include "RenderGeometryUtils.h"
#include "RenderShaderUtils.h"
#include "RenderVoxelMeshInstance.h"
#include "../Voxels/VoxelChunk.h"
#include "../Voxels/VoxelUtils.h"
#include "../World/Chunk.h"

#include "components/utilities/Buffer3D.h"

class Renderer;

class RenderVoxelChunk final : public Chunk
{
public:
	static constexpr RenderVoxelMeshInstID AIR_MESH_INST_ID = 0;

	std::vector<RenderVoxelMeshInstance> meshInsts;
	std::unordered_map<VoxelChunk::VoxelMeshDefID, RenderVoxelMeshInstID> meshInstMappings; // Note: this doesn't support VoxelIDs changing which def they point to (important if VoxelChunk::removeVoxelDef() is ever in use).
	Buffer3D<RenderVoxelMeshInstID> meshInstIDs; // Points into mesh instances.
	std::unordered_map<VoxelInt3, IndexBufferID> chasmWallIndexBufferIDsMap; // If an index buffer ID exists for a voxel, it adds a draw call for the chasm wall. IDs are owned by the render chunk manager.
	std::unordered_map<VoxelInt3, UniformBufferID> doorTransformBuffers; // Unique transform buffer per door instance, owned by this chunk.
	std::vector<RenderDrawCall> staticDrawCalls; // Most voxel geometry (walls, floors, etc.).
	std::vector<RenderDrawCall> doorDrawCalls; // All doors, open or closed.
	std::vector<RenderDrawCall> chasmDrawCalls; // Chasm walls and floors, separate from static draw calls so their textures can animate.
	std::vector<RenderDrawCall> fadingDrawCalls; // Voxels with fade shader. Note that the static draw call in the same voxel needs to be deleted to avoid a conflict in the depth buffer.

	void init(const ChunkInt2 &position, int height);
	RenderVoxelMeshInstID addMeshInst(RenderVoxelMeshInstance &&meshInst);
	void freeBuffers(Renderer &renderer);
	void clear();
};

#endif
