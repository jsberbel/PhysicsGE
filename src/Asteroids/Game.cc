#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <string>

namespace Game
{
	struct GameData
	{
		struct GameObject
		{
			glm::vec2 position, size;
			float rotation;
			float speed;
			bool hidden;
		};

		enum class AsteroidLvl : int
		{
			NONE = -1, SMALL, MEDIUM, LARGE, MAX
		};

		struct Asteroid : GameObject
		{
			AsteroidLvl lvl;
		};

		global_var const int MAX_DEFAULT_ASTEROIDS	{ 15 };
		global_var const int MAX_BULLETS			{ 40 };

		GameObject player;
		std::vector<Asteroid> asteroids { MAX_DEFAULT_ASTEROIDS };
		GameObject bullets[MAX_BULLETS];
		size_t curBullet { 0 };

		int score { 0 };
	};

	GameData::AsteroidLvl& operator--(GameData::AsteroidLvl & c) {
		using IntType = std::underlying_type<GameData::AsteroidLvl>::type;
			c = static_cast<GameData::AsteroidLvl>(static_cast<IntType>(c) - 1);
		if (c == GameData::AsteroidLvl::NONE)
			c = static_cast<GameData::AsteroidLvl>(0);
		return c;
	}

	GameData* InitGamedata(const InputData & input)
	{
		srand(static_cast<unsigned>(time(nullptr)));

		GameData * gameData = new GameData;

		// PLAYER
		{
			gameData->player = {};
			gameData->player.size = { input.windowHalfSize.x*0.04f, input.windowHalfSize.y*0.04f };
			gameData->player.hidden = false;
		}

		//ASTEROIDS
		for (auto & asteroid : gameData->asteroids)
		{
			do asteroid.position = { rand() % input.windowHalfSize.x*2 - input.windowHalfSize.x,
										rand() % input.windowHalfSize.y*2 - input.windowHalfSize.y };
			while (asteroid.position.x < input.windowHalfSize.x*0.3f && asteroid.position.x > -input.windowHalfSize.x*0.3f &&
					asteroid.position.y < input.windowHalfSize.y*0.3f && asteroid.position.y > -input.windowHalfSize.y*0.3f);

			asteroid.lvl = GameData::AsteroidLvl(rand() % int(GameData::AsteroidLvl::MAX));
			asteroid.size = { rand() % (input.windowHalfSize.x / 100) + 30.f*(int(asteroid.lvl) + 1),
								rand() % (input.windowHalfSize.y / 100) + 30.f*(int(asteroid.lvl) + 1) };

			asteroid.rotation = static_cast<float>(rand() % 360);
			asteroid.speed = rand() % 50 + 40.f*(int(GameData::AsteroidLvl::MAX) / (int(asteroid.lvl) + 1));
			asteroid.hidden = false;
		}

		//BULLETS
		for (auto & bullet : gameData->bullets)
		{
			bullet = {};
			bullet.size = { 20.f, 1.f };
			bullet.speed = 2000.0f;
			bullet.hidden = true;
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

	RenderData Update(GameData *gameData, const InputData& input)
	{
		#define player gameData->player
		#define deltaTime static_cast<float>(input.dt)

		// PROCESS INPUT
		{
			local_persist const auto PLAYER_ROTATE_SPEED = 5.f;
			local_persist const auto PLAYER_ACCELERATION = 500.f;

			if (input.buttonLeft == InputData::ButtonState::HOLD)
				player.rotation += PLAYER_ROTATE_SPEED * deltaTime;

			if (input.buttonRight == InputData::ButtonState::HOLD)
				player.rotation -= PLAYER_ROTATE_SPEED * deltaTime;

			if (input.buttonUp == InputData::ButtonState::HOLD)
				player.speed += PLAYER_ACCELERATION * deltaTime;

			if (input.buttonDown == InputData::ButtonState::HOLD)
				player.speed -= PLAYER_ACCELERATION * deltaTime;

			if (input.buttonShoot == InputData::ButtonState::UP && !player.hidden)
			{
				gameData->bullets[gameData->curBullet].position = player.position + glm::vec2{ cos(player.rotation), sin(player.rotation) } * 10.f;
				gameData->bullets[gameData->curBullet].rotation = player.rotation;
				gameData->bullets[gameData->curBullet].hidden = false;
				++gameData->curBullet = gameData->curBullet % GameData::MAX_BULLETS;
			}
		}

		// GAMEDATA -> RENDERDATA -> GL DRAW
		RenderData renderData {};

		// PROCESS ASTEROIDS AND FILL RENDERDATA
		{
			if (gameData->asteroids.empty())
			{
				gameData->asteroids.resize(static_cast<size_t>(GameData::MAX_DEFAULT_ASTEROIDS * 1.5f));
				for (auto & asteroid : gameData->asteroids)
				{
					do asteroid.position = { rand() % input.windowHalfSize.x * 2 - input.windowHalfSize.x,
						rand() % input.windowHalfSize.y * 2 - input.windowHalfSize.y };
					while (asteroid.position.x < player.position.x + input.windowHalfSize.x*0.1f &&
						   asteroid.position.x > player.position.x - input.windowHalfSize.x*0.1f &&
						   asteroid.position.y < player.position.y + input.windowHalfSize.y*0.1f &&
						   asteroid.position.y > player.position.y - input.windowHalfSize.y*0.1f);

					asteroid.lvl = GameData::AsteroidLvl(rand() % int(GameData::AsteroidLvl::MAX));
					asteroid.size = { rand() % (input.windowHalfSize.x / 100) + 30.f*(int(asteroid.lvl) + 1),
						rand() % (input.windowHalfSize.y / 100) + 30.f*(int(asteroid.lvl) + 1) };

					asteroid.rotation = static_cast<float>(rand() % 360);
					asteroid.speed = rand() % 50 + 40.f*(int(GameData::AsteroidLvl::MAX) / (int(asteroid.lvl) + 1));
					asteroid.hidden = false;
				}
			}

			for (size_t i = 0; i < gameData->asteroids.size(); ++i)
			{
				auto & asteroid = gameData->asteroids[i];

				if (asteroid.hidden) continue;

				// ASTEROID UPDATE
				{
					asteroid.position.x += cos(asteroid.rotation) * asteroid.speed * deltaTime;
					asteroid.position.y += sin(asteroid.rotation) * asteroid.speed * deltaTime;

					const auto LIMITS_OFFSET_MULTIPLIER = (int(asteroid.lvl)*0.145f) / int(GameData::AsteroidLvl::MAX) + 1.05f;

					if (asteroid.position.x < -input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER)
						asteroid.position.x = input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER;

					if (asteroid.position.x > input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER)
						asteroid.position.x = -input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER;

					if (asteroid.position.y < -input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER)
						asteroid.position.y = input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER;

					if (asteroid.position.y > input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER)
						asteroid.position.y = -input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER;
				}

				// ASTEROID FILL RENDER DATA
				RenderData::Sprite sprite = {};
				sprite.position = asteroid.position;
				sprite.size = asteroid.size;
				sprite.rotation = asteroid.rotation;
				sprite.texture = RenderData::TextureID::ASTEROID;
				switch (asteroid.lvl)
				{
					case GameData::AsteroidLvl::SMALL:
						sprite.color = { 0.2f, 0.6f, 1.f };
						break;
					case GameData::AsteroidLvl::MEDIUM:
						sprite.color = { 0.2f, 1.f, 0.2f };
						break;
					case GameData::AsteroidLvl::LARGE:
						sprite.color = { 1.f, 0.2f, 0.2f };
						break;
				}
				renderData.sprites.push_back(sprite);

				// ASTEROID COLLIDES WITH PLAYER
				auto dX = player.position.x - asteroid.position.x;
				auto dY = player.position.y - asteroid.position.y;

				if (sqrt(dX * dX + dY * dY) < (asteroid.size.x + player.size.x))
					asteroid.hidden = player.hidden = true;

				// CHECK BULLETS
				for (auto &bullet : gameData->bullets)
				{
					if (bullet.hidden) continue;

					// CHECK BULLET COLLIDES WITH ASTEROID
					dX = bullet.position.x - asteroid.position.x;
					dY = bullet.position.y - asteroid.position.y;

					if (sqrt(dX * dX + dY * dY) < (asteroid.size.x + bullet.size.x))
					{
						bullet.hidden = true;
						++gameData->score;

						if (asteroid.lvl == GameData::AsteroidLvl::SMALL)
						{
							gameData->asteroids.erase(gameData->asteroids.begin() + i);
							--i;
						}
						else
						{
							--asteroid.lvl;
							asteroid.size = { rand() % (input.windowHalfSize.x / 100) + 30.f*(int(asteroid.lvl) + 1),
								rand() % (input.windowHalfSize.y / 100) + 30.f*(int(asteroid.lvl) + 1) };

							GameData::Asteroid newAsteroid {};
							newAsteroid.position = asteroid.position * 1.2f;
							newAsteroid.lvl = asteroid.lvl;
							newAsteroid.size = { rand() % (input.windowHalfSize.x / 100) + 30.f*(int(newAsteroid.lvl) + 1),
								rand() % (input.windowHalfSize.y / 100) + 30.f*(int(newAsteroid.lvl) + 1) };
							newAsteroid.rotation = static_cast<float>(rand() % 360);
							newAsteroid.speed = rand() % 50 + 50.f;
							newAsteroid.hidden = false;
							gameData->asteroids.push_back(newAsteroid);
						}
						break;
					}
				}
			}
		}

		// PROCESS BULLETS
		for (auto &bullet : gameData->bullets)
		{
			if (bullet.hidden) continue;
			// BULLET UPDATE
			{
				bullet.position.x += cos(bullet.rotation) * bullet.speed * deltaTime;
				bullet.position.y += sin(bullet.rotation) * bullet.speed * deltaTime;
			}

			// BULLET FILL RENDER DATA
			RenderData::Sprite sprite = {};
			sprite.position = bullet.position;
			sprite.size = bullet.size;
			sprite.rotation = bullet.rotation;
			sprite.texture = RenderData::TextureID::PIXEL;
			sprite.color = { 1.f, 0.1f, 0.4f };
			renderData.sprites.push_back(sprite);
		}

		//PROCESS PLAYER AND FILL RENDER DATA
		if (!player.hidden)
		{
			player.speed *= 1 - deltaTime;
			player.position.x += cos(player.rotation) * player.speed * deltaTime;
			player.position.y += sin(player.rotation) * player.speed * deltaTime;

			local_persist const auto LIMITS_OFFSET_MULTIPLIER = 1.05f;

			if (player.position.x < -input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER)
				player.position.x = input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER;

			if (player.position.x > input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER)
				player.position.x = -input.windowHalfSize.x*LIMITS_OFFSET_MULTIPLIER;

			if (player.position.y < -input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER)
				player.position.y = input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER;

			if (player.position.y > input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER)
				player.position.y = -input.windowHalfSize.y*LIMITS_OFFSET_MULTIPLIER;

			RenderData::Sprite sprite {};
			sprite.position = player.position;
			sprite.size = player.size;
			sprite.rotation = player.rotation;
			sprite.texture = RenderData::TextureID::SPACESHIP;
			sprite.color = { 1.f, 0.8f, 0.2f };
			renderData.sprites.push_back(sprite);
		}

		// PROCESS TEXTS
		{
			RenderData::Text text {};
			text.msg = "ASTEROIDS";
			text.position = { -input.windowHalfSize.x + 25, input.windowHalfSize.y - 60 };
			text.scale = 1.f;
			text.color = { 0.1f, 0.7f, 0.9f };
			renderData.texts.push_back(text);

			text = {};
			text.msg = "Score: " + std::to_string(gameData->score);
			text.position = { input.windowHalfSize.x - 250, input.windowHalfSize.y - 60 };
			text.scale = 1.f;
			text.color = { 1.f, 0.8f, 0.2f };
			renderData.texts.push_back(text);
		}
		
		return renderData;

		#undef player
		#undef deltaTime
	}
}

