#include "Game.hh"

namespace Game
{
	struct GameData
	{
		glm::vec2 position;
	};

	GameData* CreateGameData()
	{
		return new GameData;
	}

	void Update(GameData &gameData, const InputData &input, RenderData &renderData_)
	{
		gameData.position.x = static_cast<float>(input.horizontal * input.dt);
		gameData.position.y = static_cast<float>(input.vertical * input.dt);
		renderData_.position = gameData.position;
	}

	void DeleteGameData(GameData *gameData)
	{
		delete gameData;
	}
}

