#pragma once

#include "glm/glm.hpp"

#include "TaskManager.hh"
#include "Utils.inl.hh"

namespace Game
{
	static constexpr auto MaxFPS = 60;
	static constexpr auto MaxGameObjects = 100'000u;
	static constexpr auto GameObjectScale = 0.5f;
	struct GameData;

	struct InputData
	{
		iVec2 windowHalfSize;

		float dt;

		enum class ButtonState
		{
			NONE, DOWN, HOLD, UP
		} 
		mouseButtonL, mouseButtonR;

		bool isZooming;
		float mouseWheelZoom;

		struct { long x, y; } mousePosition;

		static constexpr ButtonState ProcessKey(const bool &prevKey, const bool &nowKey);
	};

	struct RenderData
	{
		enum class TextureID
		{
			BALL_WHITE,
			MAX_BALLS,
			PIXEL,
			DEARIMGUI,
			COUNT
		};

		TextureID texture;
		glm::mat4 modelMatrices[MaxGameObjects];
		glm::vec4 colors[MaxGameObjects];
	};

	GameData* InitGamedata (const InputData & input);
	void Update (GameData & gameData, RenderData & renderData, const InputData & inputData, const Utilities::TaskManager::JobContext &context);
	void FinalizeGameData (GameData *& gameData);
}
