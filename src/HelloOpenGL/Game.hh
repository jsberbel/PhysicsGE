#pragma once
#include "glm/glm.hpp"

namespace Game
{
	struct GameData;
	struct InputData
	{
		double dt;
		double horizontal, vertical;
	};

	struct RenderData
	{
		glm::vec2 position;
	};

	GameData* CreateGameData();

	void Update(GameData &gameData, const InputData &input, RenderData &renderData_);
	
	void DeleteGameData(GameData* gameData);
}

// ImGUI