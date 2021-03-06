#include "MapLogicController.h"
#include "../Assets/ArenaPaletteName.h"
#include "../Audio/MusicUtils.h"
#include "../Entities/EntityType.h"
#include "../Game/Game.h"
#include "../Interface/WorldMapPanel.h"
#include "../UI/TextBox.h"
#include "../World/MapType.h"
#include "../World/SkyUtils.h"
#include "../World/VoxelFacing3D.h"

void MapLogicController::handleNightLightChange(Game &game, bool active)
{
	auto &renderer = game.getRenderer();
	GameState &gameState = game.getGameState();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	auto &entityManager = levelInst.getEntityManager();
	const auto &entityDefLibrary = game.getEntityDefinitionLibrary();

	// Turn streetlights on or off.
	Buffer<Entity*> entityBuffer(entityManager.getCountOfType(EntityType::Static));
	const int entityCount = entityManager.getEntitiesOfType(
		EntityType::Static, entityBuffer.get(), entityBuffer.getCount());

	for (int i = 0; i < entityCount; i++)
	{
		Entity *entity = entityBuffer.get(i);
		const EntityDefID defID = entity->getDefinitionID();
		const EntityDefinition &entityDef = entityManager.getEntityDef(defID, entityDefLibrary);

		if (EntityUtils::isStreetlight(entityDef))
		{
			const std::string &newStateName = active ?
				EntityAnimationUtils::STATE_ACTIVATED : EntityAnimationUtils::STATE_IDLE;

			const EntityAnimationDefinition &animDef = entityDef.getAnimDef();
			const std::optional<int> newStateIndex = animDef.tryGetStateIndex(newStateName.c_str());
			if (!newStateIndex.has_value())
			{
				DebugLogWarning("Missing entity animation state \"" + newStateName + "\".");
				continue;
			}

			EntityAnimationInstance &animInst = entity->getAnimInstance();
			animInst.setStateIndex(*newStateIndex);
		}
	}

	TextureManager &textureManager = game.getTextureManager();
	const std::string paletteName = ArenaPaletteName::Default;
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteName.c_str());
	if (!paletteID.has_value())
	{
		DebugCrash("Couldn't get palette \"" + paletteName + "\".");
	}

	const Palette &palette = textureManager.getPaletteHandle(*paletteID);
	renderer.setNightLightsActive(active, palette);
}

void MapLogicController::handleTriggers(Game &game, const CoordInt3 &coord, TextBox &triggerTextBox)
{
	GameState &gameState = game.getGameState();
	MapInstance &mapInst = gameState.getActiveMapInst();
	LevelInstance &levelInst = mapInst.getActiveLevel();
	ChunkManager &chunkManager = levelInst.getChunkManager();
	Chunk *chunkPtr = chunkManager.tryGetChunk(coord.chunk);
	DebugAssert(chunkPtr != nullptr);

	const TriggerDefinition *triggerDef = chunkPtr->tryGetTrigger(coord.voxel);
	if (triggerDef != nullptr)
	{
		if (triggerDef->hasSoundDef())
		{
			const TriggerDefinition::SoundDef &soundDef = triggerDef->getSoundDef();
			const std::string &soundFilename = soundDef.getFilename();

			// Play the sound.
			auto &audioManager = game.getAudioManager();
			audioManager.playSound(soundFilename);
		}

		if (triggerDef->hasTextDef())
		{
			const TriggerDefinition::TextDef &textDef = triggerDef->getTextDef();
			const VoxelInt3 &voxel = coord.voxel;
			const VoxelInstance *triggerInst = chunkPtr->tryGetVoxelInst(voxel, VoxelInstance::Type::Trigger);
			const bool hasBeenDisplayed = (triggerInst != nullptr) && triggerInst->getTriggerState().isTriggered();
			const bool canDisplay = !textDef.isDisplayedOnce() || !hasBeenDisplayed;

			if (canDisplay)
			{
				// Ignore the newline at the end.
				const std::string &textDefText = textDef.getText();
				const std::string text = textDefText.substr(0, textDefText.size() - 1);
				triggerTextBox.setText(text);
				gameState.setTriggerTextDuration(text);

				// Set the text trigger as activated (regardless of whether or not it's single-shot, just
				// for consistency).
				if (triggerInst == nullptr)
				{
					constexpr bool triggered = true;
					VoxelInstance newTriggerInst = VoxelInstance::makeTrigger(voxel.x, voxel.y, voxel.z, triggered);
					chunkPtr->addVoxelInst(std::move(newTriggerInst));
				}
			}
		}
	}
}

void MapLogicController::handleMapTransition(Game &game, const Physics::Hit &hit, 
	const TransitionDefinition &transitionDef)
{
	const TransitionType transitionType = transitionDef.getType();
	DebugAssert(transitionType != TransitionType::LevelChange);

	DebugAssert(hit.getType() == Physics::Hit::Type::Voxel);
	const Physics::Hit::VoxelHit &voxelHit = hit.getVoxelHit();
	const CoordInt3 hitCoord(hit.getCoord().chunk, voxelHit.voxel);

	auto &gameState = game.getGameState();
	auto &textureManager = game.getTextureManager();
	auto &renderer = game.getRenderer();
	const MapDefinition &activeMapDef = gameState.getActiveMapDef();
	const MapType activeMapType = activeMapDef.getMapType();
	MapInstance &activeMapInst = gameState.getActiveMapInst();
	LevelInstance &activeLevelInst = activeMapInst.getActiveLevel();

	const LocationDefinition &locationDef = gameState.getLocationDefinition();
	DebugAssert(locationDef.getType() == LocationDefinition::Type::City);
	const LocationDefinition::CityDefinition &cityDef = locationDef.getCityDefinition();

	// Decide based on the active world type.
	if (activeMapType == MapType::Interior)
	{
		DebugAssert(transitionType == TransitionType::ExitInterior);

		// @temp: temporary condition while test interiors are allowed on the main menu.
		if (!gameState.isActiveMapNested())
		{
			DebugLogWarning("Test interiors have no exterior.");
			return;
		}

		// Leave the interior and go to the saved exterior.
		const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
		if (!gameState.tryPopMap(game.getEntityDefinitionLibrary(), game.getBinaryAssetLibrary(),
			textureManager, renderer))
		{
			DebugCrash("Couldn't leave interior.");
		}

		// Change to exterior music.
		const auto &clock = gameState.getClock();
		const MusicLibrary &musicLibrary = game.getMusicLibrary();
		const MusicDefinition *musicDef = [&game, &gameState, &musicLibrary]()
		{
			if (!gameState.nightMusicIsActive())
			{
				const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
				return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
					game.getRandom(), [&weatherDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Weather);
					const auto &weatherMusicDef = def.getWeatherMusicDefinition();
					return weatherMusicDef.weatherDef == weatherDef;
				});
			}
			else
			{
				return musicLibrary.getRandomMusicDefinition(MusicDefinition::Type::Night, game.getRandom());
			}
		}();

		if (musicDef == nullptr)
		{
			DebugLogWarning("Missing exterior music.");
		}

		// Only play jingle if the exterior is inside the city.
		const MusicDefinition *jingleMusicDef = nullptr;
		if (gameState.getActiveMapDef().getMapType() == MapType::City)
		{
			jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
				game.getRandom(), [&cityDef](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
				const auto &jingleMusicDef = def.getJingleMusicDefinition();
				return (jingleMusicDef.cityType == cityDef.type) &&
					(jingleMusicDef.climateType == cityDef.climateType);
			});

			if (jingleMusicDef == nullptr)
			{
				DebugLogWarning("Missing jingle music.");
			}
		}

		AudioManager &audioManager = game.getAudioManager();
		audioManager.setMusic(musicDef, jingleMusicDef);
	}
	else
	{
		// Either city or wilderness. If the transition is for an interior, enter it. If it's the city gates,
		// toggle between city and wilderness.
		if (transitionType == TransitionType::EnterInterior)
		{
			const CoordInt3 returnCoord = [&voxelHit, &hitCoord]()
			{
				const VoxelInt3 delta = [&voxelHit, &hitCoord]()
				{
					// Assuming this is a wall voxel.
					DebugAssert(voxelHit.facing.has_value());
					const VoxelFacing3D facing = *voxelHit.facing;

					if (facing == VoxelFacing3D::PositiveX)
					{
						return VoxelInt3(1, 0, 0);
					}
					else if (facing == VoxelFacing3D::NegativeX)
					{
						return VoxelInt3(-1, 0, 0);
					}
					else if (facing == VoxelFacing3D::PositiveZ)
					{
						return VoxelInt3(0, 0, 1);
					}
					else if (facing == VoxelFacing3D::NegativeZ)
					{
						return VoxelInt3(0, 0, -1);
					}
					else
					{
						DebugUnhandledReturnMsg(VoxelInt3, std::to_string(static_cast<int>(facing)));
					}
				}();

				return hitCoord + delta;
			}();

			const TransitionDefinition::InteriorEntranceDef &interiorEntranceDef = transitionDef.getInteriorEntrance();
			const MapGeneration::InteriorGenInfo &interiorGenInfo = interiorEntranceDef.interiorGenInfo;

			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			if (!gameState.tryPushInterior(interiorGenInfo, returnCoord, game.getCharacterClassLibrary(),
				game.getEntityDefinitionLibrary(), binaryAssetLibrary, textureManager, renderer))
			{
				DebugLogError("Couldn't push new interior.");
				return;
			}

			// Change to interior music.
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition::InteriorMusicDefinition::Type interiorMusicType =
				MusicUtils::getInteriorMusicType(interiorGenInfo.getInteriorType());
			const MusicDefinition *musicDef = musicLibrary.getRandomMusicDefinitionIf(
				MusicDefinition::Type::Interior, game.getRandom(), [interiorMusicType](const MusicDefinition &def)
			{
				DebugAssert(def.getType() == MusicDefinition::Type::Interior);
				const auto &interiorMusicDef = def.getInteriorMusicDefinition();
				return interiorMusicDef.type == interiorMusicType;
			});

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing interior music.");
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef);
		}
		else if (transitionType == TransitionType::CityGate)
		{
			// City gate transition.
			const auto &binaryAssetLibrary = game.getBinaryAssetLibrary();
			const ProvinceDefinition &provinceDef = gameState.getProvinceDefinition();
			const LocationDefinition &locationDef = gameState.getLocationDefinition();
			const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
			const int currentDay = gameState.getDate().getDay();
			const int starCount = SkyUtils::getStarCountFromDensity(game.getOptions().getMisc_StarDensity());

			if (activeMapType == MapType::City)
			{
				// From city to wilderness. Use the gate position to determine where to put the player.

				// The voxel face that was hit determines where to put the player relative to the gate.
				const VoxelInt2 transitionDir = [&voxelHit]()
				{
					// Assuming this is a wall voxel.
					DebugAssert(voxelHit.facing.has_value());
					const VoxelFacing3D facing = *voxelHit.facing;

					if (facing == VoxelFacing3D::PositiveX)
					{
						return VoxelUtils::North;
					}
					else if (facing == VoxelFacing3D::NegativeX)
					{
						return VoxelUtils::South;
					}
					else if (facing == VoxelFacing3D::PositiveZ)
					{
						return VoxelUtils::East;
					}
					else if (facing == VoxelFacing3D::NegativeZ)
					{
						return VoxelUtils::West;
					}
					else
					{
						DebugUnhandledReturnMsg(VoxelInt2, std::to_string(static_cast<int>(facing)));
					}
				}();

				const auto &exeData = binaryAssetLibrary.getExeData();
				Buffer2D<ArenaWildUtils::WildBlockID> wildBlockIDs =
					ArenaWildUtils::generateWildernessIndices(cityDef.wildSeed, exeData.wild);

				MapGeneration::WildGenInfo wildGenInfo;
				wildGenInfo.init(std::move(wildBlockIDs), cityDef, cityDef.citySeed);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Use current weather.
				const WeatherDefinition &overrideWeather = weatherDef;

				// Calculate wilderness position based on the gate's voxel in the city.
				const CoordInt3 startCoord = [&hitCoord, &transitionDir]()
				{
					// Origin of the city in the wilderness.
					const ChunkInt2 wildCityChunk(ArenaWildUtils::CITY_ORIGIN_CHUNK_X, ArenaWildUtils::CITY_ORIGIN_CHUNK_Z);

					// Player position bias based on selected gate face.
					const VoxelInt3 offset(transitionDir.x, 0, transitionDir.y);

					return CoordInt3(wildCityChunk + hitCoord.chunk, hitCoord.voxel + offset);
				}();

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				if (!gameState.trySetWilderness(wildGenInfo, skyGenInfo, overrideWeather, startCoord,
					worldMapLocationIDs, game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(),
					binaryAssetLibrary, textureManager, renderer))
				{
					DebugLogError("Couldn't switch from city to wilderness for \"" + locationDef.getName() + "\".");
					return;
				}
			}
			else if (activeMapType == MapType::Wilderness)
			{
				// From wilderness to city.
				Buffer<uint8_t> reservedBlocks = [&cityDef]()
				{
					DebugAssert(cityDef.reservedBlocks != nullptr);
					Buffer<uint8_t> buffer(static_cast<int>(cityDef.reservedBlocks->size()));
					std::copy(cityDef.reservedBlocks->begin(), cityDef.reservedBlocks->end(), buffer.get());
					return buffer;
				}();

				const std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride> mainQuestTempleOverride =
					[&cityDef]() -> std::optional<LocationDefinition::CityDefinition::MainQuestTempleOverride>
				{
					if (cityDef.hasMainQuestTempleOverride)
					{
						return cityDef.mainQuestTempleOverride;
					}
					else
					{
						return std::nullopt;
					}
				}();

				MapGeneration::CityGenInfo cityGenInfo;
				cityGenInfo.init(std::string(cityDef.mapFilename), std::string(cityDef.typeDisplayName),
					cityDef.type, cityDef.citySeed, cityDef.rulerSeed, provinceDef.getRaceID(), cityDef.premade,
					cityDef.coastal, cityDef.rulerIsMale, cityDef.palaceIsMainQuestDungeon, std::move(reservedBlocks),
					mainQuestTempleOverride, cityDef.blockStartPosX, cityDef.blockStartPosY,
					cityDef.cityBlocksPerSide);

				SkyGeneration::ExteriorSkyGenInfo skyGenInfo;
				skyGenInfo.init(cityDef.climateType, weatherDef, currentDay, starCount, cityDef.citySeed,
					cityDef.skySeed, provinceDef.hasAnimatedDistantLand());

				// Use current weather.
				const WeatherDefinition &overrideWeather = weatherDef;

				// No need to change world map location here.
				const std::optional<GameState::WorldMapLocationIDs> worldMapLocationIDs;

				if (!gameState.trySetCity(cityGenInfo, skyGenInfo, overrideWeather, worldMapLocationIDs,
					game.getCharacterClassLibrary(), game.getEntityDefinitionLibrary(), binaryAssetLibrary,
					game.getTextAssetLibrary(), textureManager, renderer))
				{
					DebugLogError("Couldn't switch from wilderness to city for \"" + locationDef.getName() + "\".");
					return;
				}
			}
			else
			{
				DebugLogError("Map type \"" + std::to_string(static_cast<int>(activeMapType)) +
					"\" does not support city gate transitions.");
				return;
			}

			// Reset the current music (even if it's the same one).
			const MusicLibrary &musicLibrary = game.getMusicLibrary();
			const MusicDefinition *musicDef = [&game, &gameState, &musicLibrary]()
			{
				if (!gameState.nightMusicIsActive())
				{
					const WeatherDefinition &weatherDef = gameState.getWeatherDefinition();
					return musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Weather,
						game.getRandom(), [&weatherDef](const MusicDefinition &def)
					{
						DebugAssert(def.getType() == MusicDefinition::Type::Weather);
						const auto &weatherMusicDef = def.getWeatherMusicDefinition();
						return weatherMusicDef.weatherDef == weatherDef;
					});
				}
				else
				{
					return musicLibrary.getRandomMusicDefinition(
						MusicDefinition::Type::Night, game.getRandom());
				}
			}();

			if (musicDef == nullptr)
			{
				DebugLogWarning("Missing exterior music.");
			}

			// Only play jingle when going wilderness to city.
			const MusicDefinition *jingleMusicDef = nullptr;
			if (activeMapType == MapType::Wilderness)
			{
				jingleMusicDef = musicLibrary.getRandomMusicDefinitionIf(MusicDefinition::Type::Jingle,
					game.getRandom(), [&cityDef](const MusicDefinition &def)
				{
					DebugAssert(def.getType() == MusicDefinition::Type::Jingle);
					const auto &jingleMusicDef = def.getJingleMusicDefinition();
					return (jingleMusicDef.cityType == cityDef.type) &&
						(jingleMusicDef.climateType == cityDef.climateType);
				});

				if (jingleMusicDef == nullptr)
				{
					DebugLogWarning("Missing jingle music.");
				}
			}

			AudioManager &audioManager = game.getAudioManager();
			audioManager.setMusic(musicDef, jingleMusicDef);
		}
		else
		{
			DebugNotImplementedMsg(std::to_string(static_cast<int>(transitionType)));
		}
	}
}

void MapLogicController::handleLevelTransition(Game &game, const CoordInt3 &playerCoord,
	const CoordInt3 &transitionCoord)
{
	auto &gameState = game.getGameState();

	// Level transitions are always between interiors.
	const MapDefinition &interiorMapDef = gameState.getActiveMapDef();
	DebugAssert(interiorMapDef.getMapType() == MapType::Interior);

	MapInstance &interiorMapInst = gameState.getActiveMapInst();
	const LevelInstance &level = interiorMapInst.getActiveLevel();
	const ChunkManager &chunkManager = level.getChunkManager();
	const Chunk *chunkPtr = chunkManager.tryGetChunk(transitionCoord.chunk);
	DebugAssert(chunkPtr != nullptr);

	const VoxelInt3 &transitionVoxel = transitionCoord.voxel;
	if (!chunkPtr->isValidVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z))
	{
		// Not in the chunk.
		return;
	}

	// Get the voxel definition associated with the voxel.
	const VoxelDefinition &voxelDef = [chunkPtr, &transitionVoxel]()
	{
		const Chunk::VoxelID voxelID = chunkPtr->getVoxel(transitionVoxel.x, transitionVoxel.y, transitionVoxel.z);
		return chunkPtr->getVoxelDef(voxelID);
	}();

	// If the associated voxel data is a wall, then it might be a transition voxel.
	if (voxelDef.type == ArenaTypes::VoxelType::Wall)
	{
		const TransitionDefinition *transitionDef = chunkPtr->tryGetTransition(transitionCoord.voxel);
		if (transitionDef != nullptr)
		{
			// The direction from a level up/down voxel to where the player should end up after
			// going through. In other words, it points to the destination voxel adjacent to the
			// level up/down voxel.
			const VoxelDouble3 dirToNewVoxel = [&playerCoord, &transitionCoord]()
			{
				const VoxelInt3 diff = transitionCoord - playerCoord;

				// @todo: this probably isn't robust enough. Maybe also check the player's angle
				// of velocity with angles to the voxel's corners to get the "arrival vector"
				// and thus the "near face" that is intersected, because this method doesn't
				// handle the player coming in at a diagonal.

				// Check which way the player is going and get the reverse of it.
				if (diff.x > 0)
				{
					// From south to north.
					return -Double3::UnitX;
				}
				else if (diff.x < 0)
				{
					// From north to south.
					return Double3::UnitX;
				}
				else if (diff.z > 0)
				{
					// From west to east.
					return -Double3::UnitZ;
				}
				else if (diff.z < 0)
				{
					// From east to west.
					return Double3::UnitZ;
				}
				else
				{
					throw DebugException("Bad player transition voxel.");
				}
			}();

			// Player destination after going through a level up/down voxel.
			auto &player = gameState.getPlayer();
			const VoxelDouble3 transitionVoxelCenter = VoxelUtils::getVoxelCenter(transitionCoord.voxel);
			const CoordDouble3 destinationCoord = ChunkUtils::recalculateCoord(
				transitionCoord.chunk, transitionVoxelCenter + dirToNewVoxel);

			// Lambda for transitioning the player to the given level.
			auto switchToLevel = [&game, &gameState, &interiorMapDef, &interiorMapInst, &player, &destinationCoord,
				&dirToNewVoxel](int levelIndex)
			{
				// Clear all open doors and fading voxels in the level the player is switching away from.
				// @todo: why wouldn't it just clear them when it gets switched to in setActive()?
				auto &oldActiveLevel = interiorMapInst.getActiveLevel();

				// @todo: find a modern equivalent for this w/ either the LevelInstance or ChunkManager.
				//oldActiveLevel.clearTemporaryVoxelInstances();

				// Select the new level.
				interiorMapInst.setActiveLevelIndex(levelIndex, interiorMapDef);

				// Set the new level active in the renderer.
				auto &newActiveLevel = interiorMapInst.getActiveLevel();
				auto &newActiveSky = interiorMapInst.getActiveSky();

				WeatherDefinition weatherDef;
				weatherDef.initClear();

				const std::optional<CitizenUtils::CitizenGenInfo> citizenGenInfo; // Not used with interiors.

				// @todo: should this be called differently so it doesn't badly influence data for the rest of
				// this frame? Level changing should be done earlier I think.
				auto &textureManager = game.getTextureManager();
				auto &renderer = game.getRenderer();
				if (!newActiveLevel.trySetActive(weatherDef, gameState.nightLightsAreActive(), levelIndex,
					interiorMapDef, citizenGenInfo, textureManager, renderer))
				{
					DebugCrash("Couldn't set new level active in renderer.");
				}

				if (!newActiveSky.trySetActive(levelIndex, interiorMapDef, textureManager, renderer))
				{
					DebugCrash("Couldn't set new sky active in renderer.");
				}

				// Move the player to where they should be in the new level.
				const VoxelDouble3 playerDestinationPoint(
					destinationCoord.point.x,
					newActiveLevel.getCeilingScale() + Player::HEIGHT,
					destinationCoord.point.z);
				const CoordDouble3 playerDestinationCoord(destinationCoord.chunk, playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.lookAt(player.getPosition() + dirToNewVoxel);
				player.setVelocityToZero();

				EntityGeneration::EntityGenInfo entityGenInfo;
				entityGenInfo.init(gameState.nightLightsAreActive());

				// Tick the level's chunk manager once during initialization so the renderer is passed valid
				// chunks this frame.
				constexpr double dummyDeltaTime = 0.0;
				const int chunkDistance = game.getOptions().getMisc_ChunkDistance();
				newActiveLevel.update(dummyDeltaTime, game, player.getPosition(), levelIndex, interiorMapDef,
					entityGenInfo, citizenGenInfo, chunkDistance, game.getEntityDefinitionLibrary(),
					game.getBinaryAssetLibrary(), game.getTextureManager(), game.getAudioManager());
			};

			// Lambda for opening the world map when the player enters a transition voxel
			// that will "lead to the surface of the dungeon".
			auto switchToWorldMap = [&playerCoord, &game, &player]()
			{
				// Move player to center of previous voxel in case they change their mind
				// about fast traveling. Don't change their direction.
				const VoxelInt2 playerVoxelXZ(playerCoord.voxel.x, playerCoord.voxel.z);
				const VoxelDouble2 playerVoxelCenterXZ = VoxelUtils::getVoxelCenter(playerVoxelXZ);
				const VoxelDouble3 playerDestinationPoint(
					playerVoxelCenterXZ.x,
					player.getPosition().point.y,
					playerVoxelCenterXZ.y);
				const CoordDouble3 playerDestinationCoord(playerCoord.chunk, playerDestinationPoint);
				player.teleport(playerDestinationCoord);
				player.setVelocityToZero();

				game.setPanel<WorldMapPanel>();
			};

			// See if it's a level up or level down transition. Ignore other transition types.
			if (transitionDef->getType() == TransitionType::LevelChange)
			{
				const TransitionDefinition::LevelChangeDef &levelChangeDef = transitionDef->getLevelChange();
				if (levelChangeDef.isLevelUp)
				{
					// Level up transition. If the custom function has a target, call it and reset it (necessary
					// for main quest start dungeon).
					auto &onLevelUpVoxelEnter = gameState.getOnLevelUpVoxelEnter();

					if (onLevelUpVoxelEnter)
					{
						onLevelUpVoxelEnter(game);
						onLevelUpVoxelEnter = std::function<void(Game&)>();
					}
					else if (interiorMapInst.getActiveLevelIndex() > 0)
					{
						// Decrement the world's level index and activate the new level.
						switchToLevel(interiorMapInst.getActiveLevelIndex() - 1);
					}
					else
					{
						switchToWorldMap();
					}
				}
				else
				{
					// Level down transition.
					if (interiorMapInst.getActiveLevelIndex() < (interiorMapInst.getLevelCount() - 1))
					{
						// Increment the world's level index and activate the new level.
						switchToLevel(interiorMapInst.getActiveLevelIndex() + 1);
					}
					else
					{
						switchToWorldMap();
					}
				}
			}
		}
	}
}
