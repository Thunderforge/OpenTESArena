#ifndef ENTITY_UTILS_H
#define ENTITY_UTILS_H

#include <optional>
#include <string>

#include "EntityDefinition.h"

// Entity instance handle.
using EntityID = int;

// Entity definition handle.
using EntityDefID = int;

class CharacterClassLibrary;
class EntityDefinitionLibrary;

enum class EntityType;

namespace EntityUtils
{
	EntityType getEntityTypeFromDefType(EntityDefinition::Type defType);

	// Gets the display name of the entity definition type for debugging.
	std::string defTypeToString(const EntityDefinition &entityDef);

	// Returns whether the given entity definition ID is from a level, or if it is in the
	// entity definition library.
	bool isLevelDependentDef(EntityDefID defID, const EntityDefinitionLibrary &entityDefLibrary);

	// Returns whether the given entity definition is for a streetlight. Note that wilderness streetlights
	// do not have their activation state updated in the original game like city streetlights do.
	bool isStreetlight(const EntityDefinition &entityDef);

	int getYOffset(const EntityDefinition &entityDef);

	// Returns the entity definition's light radius, if any.
	std::optional<double> tryGetLightRadius(const EntityDefinition &entityDef, bool nightLightsAreActive);

	// Gets the max width and height from the entity animation's frames.
	void getAnimationMaxDims(const EntityAnimationDefinition &animDef, double *outMaxWidth, double *outMaxHeight);

	// Returns whether the entity definition has a display name.
	bool tryGetDisplayName(const EntityDefinition &entityDef,
		const CharacterClassLibrary &charClassLibrary, std::string *outName);
}

#endif
