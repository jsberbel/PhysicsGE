#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <string>

namespace Game
{
	struct GameData
	{
		struct Ball
		{
			glm::dvec2 pos;
			glm::dvec2 vel;
			float scale;
		};

		std::vector<Ball> balls;
		std::vector<Ball> prevBalls;
	};

	GameData* InitGamedata(const InputData & input)
	{
		srand(static_cast<unsigned>(time(nullptr)));

		GameData * gameData = new GameData;

		gameData->balls.resize(10);
		for (auto & ball : gameData->balls)
		{
			ball.pos = { rand() % input.windowHalfSize.x - (input.windowHalfSize.x >> 2) , 
						 rand() % input.windowHalfSize.y - (input.windowHalfSize.y >> 2) };
			ball.vel = { rand() % 200 - 100, rand() % 200 - 100 };
			ball.scale = 50.f;
		}

		return gameData;
	}

	void FinalizeGameData(GameData * gameData)
	{
		if (gameData != nullptr)
		{
			delete gameData;
			gameData = nullptr;
		}
	}

	InputData::ButtonState InputData::ProcessKey(const bool & prevKey, const bool & nowKey)
	{
		if (!prevKey && nowKey)
			return ButtonState::DOWN;

		if (prevKey && !nowKey)
			return ButtonState::UP;

		if (prevKey && nowKey)
			return ButtonState::HOLD;

		return ButtonState::NONE;
	}

	RenderData Update(GameData & gameData, const InputData& inputData)
	{
		gameData.prevBalls = std::move(gameData.balls);
		gameData.balls.clear();

		// update balls
		for (const auto& ball : gameData.prevBalls)
		{
			glm::dvec2 f = {};
			GameData::Ball ballNext = {};
			ballNext.pos = ball.pos + ball.vel * inputData.dt + f * (0.5 * inputData.dt * inputData.dt);
			ballNext.vel = ball.vel + f * inputData.dt;
			ballNext.scale = 50.f;

			if (ballNext.pos.x + ball.scale > inputData.windowHalfSize.x || ballNext.pos.x - ball.scale < -inputData.windowHalfSize.x)
				ballNext.vel.x = -ballNext.vel.x;
			if (ballNext.pos.y + ball.scale > inputData.windowHalfSize.y || ballNext.pos.y - ball.scale< -inputData.windowHalfSize.y)
				ballNext.vel.y = -ballNext.vel.y;

			gameData.balls.push_back(ballNext);
		}

		RenderData renderData {};
		for (const auto& ball : gameData.balls)
		{
			RenderData::Sprite sprite {};
			sprite.position = ball.pos;
			sprite.size = { ball.scale, ball.scale };
			sprite.color = { 1.f, 1.f, 1.f };
			sprite.texture = RenderData::TextureID::BALL;

			renderData.sprites.push_back(sprite);
		}

		return renderData;
	}
}