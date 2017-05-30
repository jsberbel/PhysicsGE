#pragma once
#include <glm/glm.hpp>

#define internal_fn		static
#define local_persist	static
#define global_var		static

#define MAX_FPS 60

#include "TaskManager.hh"

namespace Game
{
	static constexpr unsigned MaxGameObjects = 100'000u;
	static constexpr double GameObjectScale = 1.0;
	struct GameData;

	struct InputData
	{
		glm::ivec2 windowHalfSize;

		enum class ButtonState
		{
			NONE, DOWN, HOLD, UP
		};

		double dt;

		struct {
			long x, y;
		} mousePosition;

		ButtonState mouseButtonL;
		ButtonState mouseButtonR;
		bool isZooming;
		float mouseWheelZoom;

		internal_fn ButtonState ProcessKey(const bool &prevKey, const bool &nowKey);
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

		struct Sprite
		{
			glm::vec2 position, size;
			float rotation;
			TextureID texture;
			glm::vec3 color;
		};

		struct Text
		{
			std::string msg;
			glm::vec2 position;
			float scale;
			glm::vec3 color;
		};

		TextureID texture;
		glm::mat4 modelMatrices[MaxGameObjects];
	};

	GameData* InitGamedata (const InputData & input);
	void Update (GameData & gameData, RenderData & renderData, const InputData & inputData, const Utilities::TaskManager::JobContext &context);
	void FinalizeGameData (GameData *& gameData);
}
