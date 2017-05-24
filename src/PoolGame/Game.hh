#pragma once
#include <vector>
#include <glm/glm.hpp>

#define internal_fn		static
#define local_persist	static
#define global_var		static

#define MAX_FPS 60

//#define PARALLEL

#include "TaskManager.hh"

namespace Game
{
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
			BALL_0,
			BALL_1,
			BALL_2,
			BALL_3,
			BALL_4,
			BALL_5,
			BALL_6,
			BALL_7,
			BALL_8,
			BALL_9,
			BALL_10,
			BALL_11,
			BALL_12,
			BALL_13,
			BALL_14,
			BALL_15,
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

		std::vector<Sprite> sprites;
	};

	GameData* InitGamedata (const InputData & input);
	RenderData Update (GameData & gameData, const InputData & inputData, const Utilities::TaskManager::JobContext &context);
	void FinalizeGameData (GameData *& gameData);
}
