#include <cmath>
#include <unordered_map>
#include <vector>

#include "SDL.h"

#include "AutomapPanel.h"
#include "AutomapUiController.h"
#include "AutomapUiModel.h"
#include "AutomapUiView.h"
#include "GameWorldPanel.h"
#include "../Assets/ArenaTextureName.h"
#include "../Assets/ArenaTypes.h"
#include "../Game/CardinalDirection.h"
#include "../Game/CardinalDirectionName.h"
#include "../Game/Game.h"
#include "../Game/Options.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Media/Color.h"
#include "../Media/TextureManager.h"
#include "../Rendering/ArenaRenderUtils.h"
#include "../Rendering/Renderer.h"
#include "../UI/CursorAlignment.h"
#include "../UI/FontLibrary.h"
#include "../UI/FontName.h"
#include "../UI/RichTextString.h"
#include "../UI/Surface.h"
#include "../UI/TextAlignment.h"
#include "../UI/TextBox.h"
#include "../World/ArenaWildUtils.h"
#include "../World/MapType.h"
#include "../World/VoxelDefinition.h"
#include "../World/VoxelFacing2D.h"

#include "components/debug/Debug.h"

AutomapPanel::AutomapPanel(Game &game)
	: Panel(game) { }

bool AutomapPanel::init(const CoordDouble3 &playerCoord, const VoxelDouble2 &playerDirection,
	const ChunkManager &chunkManager, const std::string &locationName)
{
	auto &game = this->getGame();

	this->locationTextBox = [&game, &locationName]()
	{
		const auto &fontLibrary = game.getFontLibrary();
		const RichTextString richText(
			locationName,
			AutomapUiView::LocationTextBoxFontName,
			AutomapUiView::LocationTextBoxFontColor,
			AutomapUiView::LocationTextBoxTextAlignment,
			fontLibrary);
		const TextBox::ShadowData shadowData(
			AutomapUiView::LocationTextBoxShadowColor,
			AutomapUiView::LocationTextBoxShadowOffset);

		return std::make_unique<TextBox>(
			AutomapUiView::LocationTextBoxCenterPoint,
			richText,
			&shadowData,
			fontLibrary,
			game.getRenderer());
	}();

	this->backToGameButton = Button<Game&>(
		AutomapUiView::BackToGameButtonCenterPoint,
		AutomapUiView::BackToGameButtonWidth,
		AutomapUiView::BackToGameButtonHeight,
		AutomapUiController::onBackToGameButtonSelected);

	const VoxelInt3 playerVoxel = VoxelUtils::pointToVoxel(playerCoord.point);
	const CoordInt2 playerCoordXZ(playerCoord.chunk, VoxelInt2(playerVoxel.x, playerVoxel.z));
	this->mapTexture = [&game, &playerDirection, &chunkManager, &playerCoordXZ]()
	{
		const CardinalDirectionName playerCompassDir = CardinalDirection::getDirectionName(playerDirection);
		const bool isWild = [&game]()
		{
			const GameState &gameState = game.getGameState();
			const MapDefinition &activeMapDef = gameState.getActiveMapDef();
			return activeMapDef.getMapType() == MapType::Wilderness;
		}();

		Texture texture = AutomapUiView::makeAutomap(playerCoordXZ, playerCompassDir, isWild,
			chunkManager, game.getRenderer());
		return texture;
	}();

	auto &textureManager = game.getTextureManager();
	const TextureAssetReference backgroundPaletteTextureAssetRef = AutomapUiView::getBackgroundPaletteTextureAssetRef();
	const std::optional<PaletteID> backgroundPaletteID = textureManager.tryGetPaletteID(backgroundPaletteTextureAssetRef);
	if (!backgroundPaletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + backgroundPaletteTextureAssetRef.filename + "\".");
		return false;
	}

	this->backgroundPaletteID = *backgroundPaletteID;

	const TextureAssetReference backgroundTextureAssetRef = AutomapUiView::getBackgroundTextureAssetRef();
	const std::optional<TextureBuilderID> backgroundTextureBuilderID = textureManager.tryGetTextureBuilderID(backgroundTextureAssetRef);
	if (!backgroundTextureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + backgroundTextureAssetRef.filename + "\".");
		return false;
	}

	this->backgroundTextureBuilderID = *backgroundTextureBuilderID;
	this->automapOffset = AutomapUiModel::makeAutomapOffset(playerCoordXZ.voxel);
	return true;
}

std::optional<Panel::CursorData> AutomapPanel::getCurrentCursor() const
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	auto &textureManager = game.getTextureManager();

	const TextureAssetReference cursorPaletteTextureAssetRef = AutomapUiView::getCursorPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(cursorPaletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogWarning("Couldn't get palette ID for \"" + cursorPaletteTextureAssetRef.filename + "\".");
		return std::nullopt;
	}

	const TextureAssetReference cursorTextureAssetRef = AutomapUiView::getCursorTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(cursorTextureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogWarning("Couldn't get texture builder ID for \"" + cursorTextureAssetRef.filename + "\".");
		return std::nullopt;
	}

	return CursorData(*textureBuilderID, *paletteID, CursorAlignment::BottomLeft);
}

void AutomapPanel::handleEvent(const SDL_Event &e)
{
	const auto &inputManager = this->getGame().getInputManager();
	bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	bool nPressed = inputManager.keyPressed(e, SDLK_n);

	if (escapePressed || nPressed)
	{
		this->backToGameButton.click(this->getGame());
	}

	bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);

	if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 mouseOriginalPoint = this->getGame().getRenderer()
			.nativeToOriginal(mousePosition);

		// Check if "Exit" was clicked.
		if (this->backToGameButton.contains(mouseOriginalPoint))
		{
			this->backToGameButton.click(this->getGame());
		}
	}

	// @todo: text events if in text mode
}

void AutomapPanel::handleMouse(double dt)
{
	const auto &inputManager = this->getGame().getInputManager();
	const bool leftClick = inputManager.mouseButtonIsDown(SDL_BUTTON_LEFT);

	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 mouseOriginalPoint = this->getGame().getRenderer()
		.nativeToOriginal(mousePosition);

	// Check if the LMB is held on one of the compass directions.
	if (leftClick)
	{
		// @todo: move to AutomapUiController

		const double scrollSpeed = AutomapUiView::ScrollSpeed * dt;

		// Modify the automap offset based on input. The directions are reversed because
		// to go right means to push the map left.
		if (AutomapUiView::CompassRightRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassLeftRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitX * scrollSpeed);
		}
		else if (AutomapUiView::CompassUpRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset + (Double2::UnitY * scrollSpeed);
		}
		else if (AutomapUiView::CompassDownRegion.contains(mouseOriginalPoint))
		{
			this->automapOffset = this->automapOffset - (Double2::UnitY * scrollSpeed);
		}
	}
}

void AutomapPanel::drawTooltip(const std::string &text, Renderer &renderer)
{
	const Texture tooltip = TextureUtils::createTooltip(text, this->getGame().getFontLibrary(), renderer);

	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);
	const int mouseX = originalPosition.x;
	const int mouseY = originalPosition.y;
	const int x = ((mouseX + 8 + tooltip.getWidth()) < ArenaRenderUtils::SCREEN_WIDTH) ?
		(mouseX + 8) : (mouseX - tooltip.getWidth());
	const int y = ((mouseY + tooltip.getHeight()) < ArenaRenderUtils::SCREEN_HEIGHT) ?
		(mouseY - 1) : (mouseY - tooltip.getHeight());

	renderer.drawOriginal(tooltip, x, y);
}

void AutomapPanel::tick(double dt)
{
	this->handleMouse(dt);
}

void AutomapPanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw automap background.
	const auto &textureManager = this->getGame().getTextureManager();
	renderer.drawOriginal(this->backgroundTextureBuilderID, this->backgroundPaletteID, textureManager);

	// Only draw the part of the automap within the drawing area.
	const Rect nativeDrawingArea = renderer.originalToNative(AutomapUiView::DrawingArea);
	renderer.setClipRect(&nativeDrawingArea.getRect());

	// Draw automap.
	constexpr double pixelSizeReal = static_cast<double>(AutomapUiView::PixelSize);
	const int offsetX = static_cast<int>(std::floor(this->automapOffset.x * pixelSizeReal));
	const int offsetY = static_cast<int>(std::floor(this->automapOffset.y * pixelSizeReal));
	const int mapX = (AutomapUiView::DrawingArea.getLeft() + (AutomapUiView::DrawingArea.getWidth() / 2)) + offsetX;
	const int mapY = (AutomapUiView::DrawingArea.getTop() + (AutomapUiView::DrawingArea.getHeight() / 2)) + offsetY;
	renderer.drawOriginal(this->mapTexture, mapX, mapY);

	// Reset renderer clipping to normal.
	renderer.setClipRect(nullptr);

	// Draw text: title.
	renderer.drawOriginal(this->locationTextBox->getTexture(),
		this->locationTextBox->getX(), this->locationTextBox->getY());

	// Check if the mouse is over the compass directions for tooltips.
	const auto &inputManager = this->getGame().getInputManager();
	const Int2 mousePosition = inputManager.getMousePosition();
	const Int2 originalPosition = renderer.nativeToOriginal(mousePosition);

	if (AutomapUiView::CompassUpRegion.contains(originalPosition))
	{
		this->drawTooltip("Up", renderer);
	}
	else if (AutomapUiView::CompassDownRegion.contains(originalPosition))
	{
		this->drawTooltip("Down", renderer);
	}
	else if (AutomapUiView::CompassLeftRegion.contains(originalPosition))
	{
		this->drawTooltip("Left", renderer);
	}
	else if (AutomapUiView::CompassRightRegion.contains(originalPosition))
	{
		this->drawTooltip("Right", renderer);
	}
}
