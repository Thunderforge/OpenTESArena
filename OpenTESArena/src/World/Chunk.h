#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <climits>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "ChunkUtils.h"
#include "Coord.h"
#include "DoorDefinition.h"
#include "LockDefinition.h"
#include "TransitionDefinition.h"
#include "TriggerDefinition.h"
#include "VoxelDefinition.h"
#include "VoxelInstance.h"
#include "VoxelUtils.h"
#include "../Math/MathUtils.h"

#include "components/utilities/Buffer3D.h"

// A 3D set of voxels for a portion of the game world.

class AudioManager;

class Chunk
{
public:
	// This type must always support at least as many bits as are needed per voxel.
	using VoxelID = uint8_t;

	using TransitionID = int;
	using TriggerID = int;
	using LockID = int;
	using BuildingNameID = int;
	using DoorID = int;
private:
	// Determines the allowed number of voxel types per chunk.
	static constexpr int BITS_PER_VOXEL = 8;
	static_assert((sizeof(VoxelID) * CHAR_BIT) >= BITS_PER_VOXEL);

	static constexpr int MAX_VOXEL_DEFS = 1 << BITS_PER_VOXEL;

	// Indices into voxel definitions.
	Buffer3D<VoxelID> voxels;

	// Voxel definitions, pointed to by voxel IDs. If the associated bool is true,
	// the voxel data is in use by the voxel grid.
	std::array<VoxelDefinition, MAX_VOXEL_DEFS> voxelDefs;
	std::array<bool, MAX_VOXEL_DEFS> activeVoxelDefs;

	// Instance data for voxels that are uniquely different in some way.
	std::vector<VoxelInstance> voxelInsts;

	// Chunk decorators.
	std::vector<TransitionDefinition> transitionDefs;
	std::vector<TriggerDefinition> triggerDefs;
	std::vector<LockDefinition> lockDefs;
	std::vector<std::string> buildingNames;
	std::vector<DoorDefinition> doorDefs;

	// Indices into chunk decorators.
	std::unordered_map<VoxelInt3, TransitionID> transitionDefIndices;
	std::unordered_map<VoxelInt3, TriggerID> triggerDefIndices;
	std::unordered_map<VoxelInt3, LockID> lockDefIndices;
	std::unordered_map<VoxelInt3, BuildingNameID> buildingNameIndices;
	std::unordered_map<VoxelInt3, DoorID> doorDefIndices;

	// Chunk coordinates in the world.
	ChunkInt2 coord;

	// Gets the voxel definitions adjacent to a voxel. Useful with context-sensitive voxels like chasms.
	// This is slightly different than the chunk manager's version since it is chunk-independent (but as
	// a result, voxels on a chunk edge must be updated by the chunk manager).
	void getAdjacentVoxelDefs(const VoxelInt3 &voxel, const VoxelDefinition **outNorth,
		const VoxelDefinition **outEast, const VoxelDefinition **outSouth, const VoxelDefinition **outWest);

	// Runs any voxel instance behavior based on its current state that cannot be done by the voxel
	// instance itself.
	void handleVoxelInstState(VoxelInstance &voxelInst, const CoordDouble3 &playerCoord,
		double ceilingScale, AudioManager &audioManager);

	// Runs any voxel instance shutdown behavior required by the given voxel instance type.
	void handleVoxelInstFinished(VoxelInstance &voxelInst, double ceilingScale, AudioManager &audioManager);

	// Runs any context-sensitive voxel instance shutdown behavior based on its current state that cannot
	// be done by the voxel instance itself. This is needed because of chasms that rely on adjacent voxels
	// for which chasm faces they have.
	void handleVoxelInstPostFinished(VoxelInstance &voxelInst, std::vector<int> &voxelInstIndicesToDestroy);
public:
	static constexpr VoxelID AIR_VOXEL_ID = 0;
	static constexpr SNInt WIDTH = ChunkUtils::CHUNK_DIM;
	static constexpr WEInt DEPTH = WIDTH;
	static_assert(MathUtils::isPowerOf2(WIDTH));

	void init(const ChunkInt2 &coord, int height);

	int getHeight() const;

	// Gets the chunk's XY coordinate in the world.
	const ChunkInt2 &getCoord() const; // @todo: rename to position or something; Coord has different meaning now.

	// Returns whether the given voxel coordinate is in the chunk.
	bool isValidVoxel(SNInt x, int y, WEInt z) const;

	// Gets the voxel ID at the given coordinate.
	VoxelID getVoxel(SNInt x, int y, WEInt z) const;

	// Gets the number of active voxel definitions.
	int getVoxelDefCount() const;

	// Gets the voxel definition associated with a voxel ID.
	const VoxelDefinition &getVoxelDef(VoxelID id) const;

	// Gets the number of voxel instances.
	int getVoxelInstCount() const;

	// Gets the voxel instance at the given index.
	VoxelInstance &getVoxelInst(int index);
	const VoxelInstance &getVoxelInst(int index) const;

	// Convenience functions for attempting to get a voxel instance at the given voxel.
	std::optional<int> tryGetVoxelInstIndex(const VoxelInt3 &voxel, VoxelInstance::Type type) const;
	VoxelInstance *tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type);
	const VoxelInstance *tryGetVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type) const;

	const TransitionDefinition *tryGetTransition(const VoxelInt3 &voxel) const;
	const TriggerDefinition *tryGetTrigger(const VoxelInt3 &voxel) const;
	const LockDefinition *tryGetLock(const VoxelInt3 &voxel) const;
	const std::string *tryGetBuildingName(const VoxelInt3 &voxel) const;
	const DoorDefinition *tryGetDoor(const VoxelInt3 &voxel) const;

	// Sets the voxel at the given coordinate.
	void setVoxel(SNInt x, int y, WEInt z, VoxelID id);

	// Attempts to add a voxel definition and returns its assigned ID.
	bool tryAddVoxelDef(VoxelDefinition &&voxelDef, VoxelID *outID);

	// Adds a voxel instance to the chunk.
	void addVoxelInst(VoxelInstance &&voxelInst);

	// Adds a chunk decorator definition to the chunk and returns its newly assigned ID.
	TransitionID addTransition(TransitionDefinition &&transition);
	TriggerID addTrigger(TriggerDefinition &&trigger);
	LockID addLock(LockDefinition &&lock);
	BuildingNameID addBuildingName(std::string &&buildingName);
	DoorID addDoorDef(DoorDefinition &&door);

	// Adds a mapping of the chunk decorator definition ID to the given voxel.
	void addTransitionPosition(TransitionID id, const VoxelInt3 &voxel);
	void addTriggerPosition(TriggerID id, const VoxelInt3 &voxel);
	void addLockPosition(LockID id, const VoxelInt3 &voxel);
	void addBuildingNamePosition(BuildingNameID id, const VoxelInt3 &voxel);
	void addDoorPosition(DoorID id, const VoxelInt3 &voxel);

	// Removes a voxel definition so its corresponding voxel ID can be reused.
	void removeVoxelDef(VoxelID id);

	// Removes a certain type of voxel instance from the given voxel (if any). This might be useful when
	// updating a chunk edge due to adjacent chunks changing.
	void removeVoxelInst(const VoxelInt3 &voxel, VoxelInstance::Type type);

	// Clears all chunk state.
	void clear();

	// Animates the chunk's voxels by delta time.
	// @todo: evaluate just letting the chunk manager do all the updating for the chunk, due to the complexity
	// of chunk perimeters, etc. and the amount of almost-identical problem solving between the two classes.
	void update(double dt, const CoordDouble3 &playerCoord, double ceilingScale, AudioManager &audioManager);
};

#endif
