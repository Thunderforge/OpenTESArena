#ifndef MAP_LOGIC_CONTROLLER_H
#define MAP_LOGIC_CONTROLLER_H

#include "../Game/Physics.h"
#include "../World/Coord.h"

class Game;
class TextBox;
class TransitionDefinition;

namespace MapLogicController
{
	// Handles changing night-light-related things on and off.
	void handleNightLightChange(Game &game, bool active);

	// Sends an "on voxel enter" message for the given voxel and triggers any text or sound events.
	void handleTriggers(Game &game, const CoordInt3 &coord, TextBox &triggerTextBox);

	// Handles the behavior for when the player activates a map transition block and transitions from one map
	// to another (i.e., from an interior to an exterior). This does not handle level transitions.
	void handleMapTransition(Game &game, const Physics::Hit &hit, const TransitionDefinition &transitionDef);

	// Checks the given transition voxel to see if it's a level transition (i.e., level up/down), and changes
	// the current level if it is.
	void handleLevelTransition(Game &game, const CoordInt3 &playerCoord, const CoordInt3 &transitionCoord);
}

#endif
