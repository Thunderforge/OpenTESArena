#include "SDL.h"

#include "LoadSavePanel.h"
#include "LoadSaveUiController.h"
#include "LoadSaveUiModel.h"
#include "LoadSaveUiView.h"
#include "../Game/Game.h"
#include "../UI/CursorData.h"

LoadSavePanel::LoadSavePanel(Game &game)
	: Panel(game) { }

bool LoadSavePanel::init(LoadSavePanel::Type type)
{
	auto &game = this->getGame();
	auto &renderer = game.getRenderer();
	const auto &fontLibrary = game.getFontLibrary();

	// Populate save slots.
	const std::vector<LoadSaveUiModel::Entry> entries = LoadSaveUiModel::getSaveEntries(game);
	for (int i = 0; i < static_cast<int>(entries.size()); i++)
	{
		const LoadSaveUiModel::Entry &entry = entries[i];
		const std::string &text = entry.displayText;
		const TextBox::InitInfo textBoxInitInfo = TextBox::InitInfo::makeWithCenter(
			text,
			LoadSaveUiView::getEntryCenterPoint(i),
			LoadSaveUiView::EntryFontName,
			LoadSaveUiView::EntryTextColor,
			LoadSaveUiView::EntryTextAlignment,
			fontLibrary);

		TextBox textBox;
		if (!textBox.init(textBoxInitInfo, text, renderer))
		{
			DebugLogError("Couldn't init load/save text box " + std::to_string(i) + ".");
			continue;
		}

		this->saveTextBoxes.emplace_back(std::move(textBox));
	}

	this->confirmButton = Button<Game&, int>(LoadSaveUiController::onEntryButtonSelected);
	this->backButton = Button<Game&>(LoadSaveUiController::onBackButtonSelected);
	this->type = type;

	return true;
}

std::optional<CursorData> LoadSavePanel::getCurrentCursor() const
{
	return this->getDefaultCursor();
}

void LoadSavePanel::handleEvent(const SDL_Event &e)
{
	auto &game = this->getGame();
	const auto &inputManager = game.getInputManager();
	const bool escapePressed = inputManager.keyPressed(e, SDLK_ESCAPE);
	const bool leftClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_LEFT);
	const bool rightClick = inputManager.mouseButtonPressed(e, SDL_BUTTON_RIGHT);

	if (escapePressed || rightClick)
	{
		this->backButton.click(game);
	}
	else if (leftClick)
	{
		const Int2 mousePosition = inputManager.getMousePosition();
		const Int2 originalPoint = game.getRenderer().nativeToOriginal(mousePosition);

		// Listen for saved game click.
		// @todo: each save entry should just be a button with its own callback based on its index
		const std::optional<int> clickedIndex = LoadSaveUiModel::getClickedIndex(originalPoint);
		if (clickedIndex.has_value())
		{
			this->confirmButton.click(game, *clickedIndex);
		}
	}
}

void LoadSavePanel::render(Renderer &renderer)
{
	// Clear full screen.
	renderer.clear();

	// Draw slots background.
	auto &textureManager = this->getGame().getTextureManager();
	const TextureAssetReference paletteTextureAssetRef = LoadSaveUiView::getPaletteTextureAssetRef();
	const std::optional<PaletteID> paletteID = textureManager.tryGetPaletteID(paletteTextureAssetRef);
	if (!paletteID.has_value())
	{
		DebugLogError("Couldn't get palette ID for \"" + paletteTextureAssetRef.filename + "\".");
		return;
	}

	const TextureAssetReference loadSaveTextureAssetRef = LoadSaveUiView::getLoadSaveTextureAssetRef();
	const std::optional<TextureBuilderID> textureBuilderID = textureManager.tryGetTextureBuilderID(loadSaveTextureAssetRef);
	if (!textureBuilderID.has_value())
	{
		DebugLogError("Couldn't get texture builder ID for \"" + loadSaveTextureAssetRef.filename + "\".");
		return;
	}

	renderer.drawOriginal(*textureBuilderID, *paletteID, textureManager);

	// Draw save text.
	for (TextBox &textBox : this->saveTextBoxes)
	{
		const Rect &textBoxRect = textBox.getRect();
		renderer.drawOriginal(textBox.getTexture(), textBoxRect.getLeft(), textBoxRect.getTop());
	}
}
