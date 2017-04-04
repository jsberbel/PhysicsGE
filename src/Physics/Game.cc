#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <string>
#include <algorithm>
#include <unordered_map>

namespace Game
{
	struct GameData
	{
		struct Line
		{
			glm::dvec2 normal;
			double distance;
		};

		struct Circle
		{
			glm::dvec2 position;
			double radius;
		};

		struct GameObject // Ball
		{
			glm::dvec2 pos;
			glm::dvec2 vel;
			double scale;
			Circle col;
			double invMass;
			inline glm::dvec2 GetExtreme(const glm::dvec2 & extreme) const
			{
				return pos + scale*extreme;
			}
		};

		std::vector<GameObject> balls;
		GameObject *whiteBall;

		struct TouchInfo
		{
			bool isTouched { false };
			glm::dvec2 posReleased {};
			glm::dvec2 posTouched {};
		} whiteBallTouchInfo;
		

		struct PossibleCollission
		{
			GameObject *a, *b;
		};

		struct ContactData
		{
			GameObject *a, *b;
			glm::dvec2 point, normal;
			double penetatrion;
			double totalInvMass;
			// --
			double restitution, friction;
		};

		struct ContactGroup
		{
			std::vector<GameObject*> objects;
			std::vector<ContactData> contacts;
		};
	};

	GameData* InitGamedata(const InputData & input)
	{
		srand(static_cast<unsigned>(time(nullptr)));

		GameData * gameData = new GameData;

		gameData->balls.resize(16);
		//for (auto & ball : gameData->balls)
		int c = 1;
		for (int i = 0; i <= 4; ++i)
		{
			for (int j = 0; j < i + 1; ++j)
			{
				gameData->balls[c].pos.x = -50 - i * int(input.windowHalfSize.x*0.12f);
				gameData->balls[c].pos.y = 0 - i * int(input.windowHalfSize.y*0.1f) + j * int(input.windowHalfSize.y*0.2f);
				c++;
			}
		}
		gameData->whiteBall = &gameData->balls[0];
		gameData->whiteBall->pos.x = input.windowHalfSize.x * 0.3f;
		gameData->whiteBall->pos.y = 0;
		
		for (int i = static_cast<int>(Game::RenderData::TextureID::BALL_0);
			 i < static_cast<int>(Game::RenderData::TextureID::MAX_BALLS);
			 ++i)
		{
			//ball.vel = glm::dvec2{ rand() % 100 + 50, rand() % 100 + 50 } * (1 - round(double(rand()) / double(RAND_MAX)) * 2.0);
			gameData->balls[i].scale = 30.0;
			gameData->balls[i].invMass = 1 / 50.0;
			gameData->balls[i].col.position = gameData->balls[i].pos;
			gameData->balls[i].col.radius = gameData->balls[i].scale;
		}

		return gameData;
	}

	void FinalizeGameData(GameData *& gameData)
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

	// Broad-Phase: "Sort & Sweep"
	//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
	//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
	//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
	//        aquests son les possibles colisions.
	std::vector<GameData::PossibleCollission> SortAndSweep(std::vector<GameData::GameObject> & gameObjects)
	{
		struct Extreme
		{
			GameData::GameObject * go;
			double p;
			bool min;
		};

		std::vector<Extreme> extremes;

		for (GameData::GameObject & go : gameObjects)
		{
			extremes.push_back(Extreme { &go, dot(go.GetExtreme({ -1.0, 0.0 }), { 1.0, 0.0 }), true });
			extremes.push_back(Extreme { &go, dot(go.GetExtreme({ +1.0, 0.0 }), { 1.0, 0.0 }), false });
		}

		std::sort(extremes.begin(), extremes.end(), [] (const Extreme & lhs, const Extreme & rhs)
		{
			return (lhs.p < rhs.p);
		});

		std::vector<GameData::PossibleCollission> result;

		for (int i = 0; i < extremes.size(); ++i)
		{
			if (extremes[i].min)
			{
				for (int j = i + 1; extremes[i].go != extremes[j].go; ++j)
				{
					if (extremes[j].min)
					{
						result.push_back(GameData::PossibleCollission { extremes[i].go, extremes[j].go });
					}
				}
			}
		}
		return result;
	}

	std::vector<GameData::ContactGroup> GenerateContactGroups(std::vector<GameData::ContactData> contactData)
	{
		// si ordenem podem "simplificar" la segona part.
		// sense probar-ho no podem saber si és més optim o no.

		//std::sort(contactData.begin(), contactData.end(), [](const GameData::ContactData& a, const GameData::ContactData& b) // aquesta lambda serveix per ordenar i és equivalent a "a < b"
		//{
		//	// ens assegurem que el contacte estigui ben generat
		//	assert(a.a < a.b || a.b != nullptr);
		//	assert(b.a < b.b || b.b != nullptr); // contactes amb l'element "b" a null son contactes amb objectes de massa infinita (parets, pex)
		//	
		//	if (a.a < b.a)
		//		return true;
		//	else if (a.a > b.a)
		//		return false;
		//	else if (a.b < b.b)
		//		return true;
		//	else
		//		return false;
		//});


		std::vector<GameData::ContactGroup> result;
		std::unordered_map<GameData::GameObject*, GameData::ContactGroup*> createdGroups;

		result.reserve(contactData.size());

		for (int i = 0; i < contactData.size(); ++i)
		{
			auto it = createdGroups.find(contactData[i].a); // busquem si ja tenim alguna colisió amb l'objecte "a"
			if (it == createdGroups.end())
			{
				it = createdGroups.find(contactData[i].b); // busquem si ja tenim alguna colisió amb l'objecte "b"

				if (it == createdGroups.end())
				{
					GameData::ContactGroup group;
					group.objects.push_back(contactData[i].a);
					group.objects.push_back(contactData[i].b);
					group.contacts.push_back(contactData[i]);

					result.push_back(group); // creem la llista de colisions nova

					createdGroups[contactData[i].a] = &result[result.size() - 1]; // guardem referència a aquesta llista
					createdGroups[contactData[i].b] = &result[result.size() - 1]; // per cada objecte per trobarla
				}
				else
				{
					it->second->contacts.push_back(contactData[i]); // afegim la colisió a la llista
					it->second->objects.push_back(contactData[i].a);
					createdGroups[contactData[i].a] = it->second; // guardem referència de l'objecte que no hem trobat abans
				}
			}
			else
			{
				GameData::ContactGroup *groupA = it->second;
				groupA->contacts.push_back(contactData[i]);

				auto itB = createdGroups.find(contactData[i].b);
				if (itB == createdGroups.end())
				{
					groupA->objects.push_back(contactData[i].b);
					createdGroups[contactData[i].b] = groupA;
				}
				else
				{
					GameData::ContactGroup *groupB = itB->second;

					if (groupA != groupB)
					{
						// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

						// 1 - copiem tot el grup anterior
						for (auto& contactData : groupB->contacts)
						{
							groupA->contacts.push_back(contactData);
						}

						// 2 - copiem els elements del segon grup i actualitzem el mapa
						//     de grups
						for (GameData::GameObject* go : groupB->objects)
						{
							if (go != contactData[i].a)
								groupA->objects.push_back(go);
							createdGroups[go] = groupA;
						}

						// 3 - marquem el grup com a buit
						groupB->objects.clear();
						groupB->contacts.clear();
					}
				}
			}
		}

		return result;
	}

	inline void SolveVelocity(GameData::ContactData * contactData)
	{
		float approachVel = glm::dot(contactData->a->vel - contactData->b->vel, glm::normalize(contactData->a->pos - contactData->b->pos));
		auto separationVel = -approachVel;
		
		if (separationVel > 0.0)
		{
			static const auto k = 0.5; // 0 - 1

			auto newSeparationVel = -k * separationVel;
			auto deltaVel = newSeparationVel - separationVel;
			auto impulse = deltaVel / contactData->totalInvMass;
			auto impulsePerIMass = contactData->normal * impulse;
			contactData->a->vel -= impulsePerIMass * contactData->a->invMass;
			contactData->b->vel += impulsePerIMass * contactData->b->invMass;
		}
	}

	inline void SolvePenetration(GameData::ContactData * contactData)
	{
		if (contactData->penetatrion > 0.0)
		{
			auto movePerIMass = contactData->normal * (glm::normalize(contactData->penetatrion) / contactData->totalInvMass);
			contactData->a->pos += movePerIMass * contactData->a->invMass;
			contactData->b->pos -= movePerIMass * contactData->b->invMass;
		}
	}

	inline bool HasCollision(const GameData::GameObject * c1, const GameData::GameObject * c2)
	{
		if (c1->pos == c2->pos && c1->vel == c2->vel)
			return false;

		auto dist = sqrt((c1->col.position.x - c2->col.position.x) * (c1->col.position.x - c2->col.position.x)
						 + (c1->col.position.y - c2->col.position.y) * (c1->col.position.y - c2->col.position.y));

		return (dist < c1->col.radius + c2->col.radius);
	}

	inline GameData::ContactData GenerateContactData(GameData::GameObject * c1, GameData::GameObject * c2)
	{
		GameData::ContactData contact;
		contact.a = c1;
		contact.b = c2;
		contact.normal = glm::normalize(c1->col.position - c2->col.position);
		contact.point = c1->pos + contact.normal * c1->col.radius;
		contact.penetatrion = (c1->col.radius + c2->col.radius) - 
			glm::distance(c1->col.position, c2->pos + contact.normal*c2->col.radius) - 
			glm::distance(c2->col.position, c1->pos - contact.normal*c1->col.radius);
		contact.totalInvMass = c1->invMass + c2->invMass;
		return contact;
	}

	inline std::vector<GameData::ContactData> FineGrained(const std::vector<GameData::PossibleCollission> & possibleCollisions)
	{
		std::vector<GameData::ContactData> contactData;
		for (auto& pc : possibleCollisions)
		{
			if (HasCollision(pc.a, pc.b))
				contactData.push_back(GenerateContactData(pc.a, pc.b));
		}
		return contactData;
	}

	void SolveCollissionGroup(const GameData::ContactGroup& contactGroup)
	{
		std::vector<GameData::ContactData> contacts = contactGroup.contacts;
		int iterations = 0;
		while (contacts.size() > 0 && iterations < contactGroup.contacts.size() * 3)
		{
			// busquem la penetració més gran
			GameData::ContactData* contactData = nullptr;
			for (GameData::ContactData& candidate : contacts)
			{
				if (contactData == nullptr || contactData->penetatrion < candidate.penetatrion)
					contactData = &candidate;
			}

			if (contactData->penetatrion < 1e-5)
				break;
			
			SolveVelocity(contactData);
			SolvePenetration(contactData);

		#if 0
			std::vector<PossibleCollission> possibleCollissions;
			for (ContactData& cd : contactGroup.contacts)
			{
				possibleCollissions.push_back({ cd.a, cd.b });
			}

			contacts = FindCollissions(possibleCollissions);
		#elif 0
			contacts = FindCollissions(contactGroup.contacts);
		#else
			// tornem a generar contactes per al grup
			contacts.clear();
			for (int i = 0; i < contactGroup.objects.size(); ++i)
			{
				for (int j = i + 1; j < contactGroup.objects.size(); ++j)
				{
					if (HasCollision(contactGroup.objects[i], contactGroup.objects[j]))
					{
						GameData::ContactData cd = GenerateContactData(contactGroup.objects[i], contactGroup.objects[j]);
						contacts.push_back(cd);
					}
				}
			}
		#endif
			++iterations;
		}
	}

	inline void UpdateBalls(std::vector<GameData::GameObject> &balls_, const InputData& inputData)
	{
		for (auto& ball : balls_)
		{
			const double k0 = 0.8, k1 = 0.1, k2 = 0.01;

			glm::dvec2 f = {};

			double brakeValue = pow(k0, inputData.dt);

			double speed = glm::length(ball.vel);
			if (speed > 0)
			{
				double fv = k1 * speed + k2 * speed * speed;
				glm::dvec2 friction = -fv * glm::normalize(ball.vel);
				f += friction;
			}

			glm::dvec2 a = f * ball.invMass;

			ball.pos += ball.vel * inputData.dt + f * (0.5 * inputData.dt * inputData.dt);
			ball.vel = brakeValue * ball.vel + a * inputData.dt;
			ball.col.position = ball.pos;

			if (glm::length(ball.vel) < 1e-3)
				ball.vel = { 0,0 };

			if (ball.pos.x + ball.scale > inputData.windowHalfSize.x)
				ball.pos.x = inputData.windowHalfSize.x - ball.scale,
				ball.vel.x = -ball.vel.x;
			if (ball.pos.x - ball.scale < -inputData.windowHalfSize.x)
				ball.pos.x = -inputData.windowHalfSize.x + ball.scale,
				ball.vel.x = -ball.vel.x;
			if (ball.pos.y + ball.scale > inputData.windowHalfSize.y)
				ball.pos.y = inputData.windowHalfSize.y - ball.scale,
				ball.vel.y = -ball.vel.y;
			if (ball.pos.y - ball.scale < -inputData.windowHalfSize.y)
				ball.pos.y = -inputData.windowHalfSize.y + ball.scale,
				ball.vel.y = -ball.vel.y;
		}
	}

	RenderData Update(GameData & gameData, const InputData& inputData)
	{
		// PROCESS INPUT
		if (inputData.mouseButtonL == InputData::ButtonState::DOWN || inputData.mouseButtonL == InputData::ButtonState::HOLD)
		{
			auto realMouseX = inputData.mousePosition.x - inputData.windowHalfSize.x;
			auto realMouseY = inputData.windowHalfSize.y - inputData.mousePosition.y;

			if (gameData.whiteBall->pos.x - gameData.whiteBall->scale < double(realMouseX) &&
				gameData.whiteBall->pos.x + gameData.whiteBall->scale > double(realMouseX) &&
				gameData.whiteBall->pos.y - gameData.whiteBall->scale < double(realMouseY) &&
				gameData.whiteBall->pos.y + gameData.whiteBall->scale > double(realMouseY))
			{
				/*char buffer[512];
				sprintf_s(buffer, "x: %f, y: %d\nx: %f, y : %d\n\n", gameData.whiteBall->pos.x, realMouseX, gameData.whiteBall->pos.y, realMouseY);
				OutputDebugStringA(buffer);*/
				//gameData.whiteBall->vel = {};
				gameData.whiteBallTouchInfo.isTouched = true;
				gameData.whiteBallTouchInfo.posTouched = gameData.whiteBall->pos;
			}
		}
		if (gameData.whiteBallTouchInfo.isTouched)
		{
			auto realMouseX = inputData.mousePosition.x - inputData.windowHalfSize.x;
			auto realMouseY = inputData.windowHalfSize.y - inputData.mousePosition.y;

			gameData.whiteBallTouchInfo.posReleased = { realMouseX, realMouseY };
			gameData.whiteBallTouchInfo.posTouched = gameData.whiteBall->pos;
			
			if (inputData.mouseButtonL == InputData::ButtonState::UP)
			{
				gameData.whiteBall->vel = -(gameData.whiteBallTouchInfo.posReleased - gameData.whiteBallTouchInfo.posTouched) * 5.0;
				gameData.whiteBallTouchInfo.isTouched = false;
			}
		}

		// UPDATE PHYSICS
		{
			// 1 - Update posicions / velocitats
			UpdateBalls(gameData.balls, inputData);

			// 2 - Generació de colisions

			//   2.1 - Broad-Phase: "Sort & Sweep"
			auto possibleCollisions = SortAndSweep(gameData.balls);

			//   2.2 - Fine-Grained
			auto contacts = FineGrained(possibleCollisions);

			//   2.3 - Sort / Generació de grups de col·lisions
			auto contactGroups = GenerateContactGroups(contacts);

			// 3 - Resolució de colisions
			for (auto & group : contactGroups)
				SolveCollissionGroup(group);
		}

		// RENDER BALLS
		RenderData renderData {};
		//for (const auto& ball : gameData.balls)
		for (int i = static_cast<int>(Game::RenderData::TextureID::BALL_0);
			 i < static_cast<int>(Game::RenderData::TextureID::MAX_BALLS);
			 ++i)
		{
			RenderData::Sprite sprite {};
			sprite.position = gameData.balls[i].pos;
			//sprite.rotation = atan2f (gameData.balls[i].vel.y, gameData.balls[i].vel.x);
			sprite.size = { gameData.balls[i].scale, gameData.balls[i].scale };
			sprite.color = { 1.f, 1.f, 1.f };
			sprite.texture = static_cast<RenderData::TextureID>(i);

			renderData.sprites.push_back(sprite);
		}

		// RENDER DEBUG LINE SHOT
		if (gameData.whiteBallTouchInfo.isTouched)
		{
			RenderData::Sprite sprite {};
			auto dir = glm::normalize(gameData.whiteBallTouchInfo.posReleased - gameData.whiteBallTouchInfo.posTouched);
			auto dist = glm::distance(gameData.whiteBallTouchInfo.posReleased, gameData.whiteBallTouchInfo.posTouched);
			sprite.position = gameData.whiteBall->pos - dir * dist;
			sprite.rotation = atan2(dir.y, dir.x);
			//sprite.rotation = glm::radians(sin(gameData.whiteBallTouchInfo.posReleased.x) + cos(gameData.whiteBallTouchInfo.posReleased.y));
			sprite.size = { dist, 3 };
			/*char buffer[512];
			sprintf_s(buffer, "x: %f\n", glm::distance(gameData.whiteBallTouchInfo.posReleased, gameData.whiteBallTouchInfo.posTouched));
			OutputDebugStringA(buffer);*/
			sprite.color = { 1.f, 0.f, 0.f };
			sprite.texture = RenderData::TextureID::PIXEL;

			renderData.sprites.push_back(sprite);
		}

		return renderData;
	}
	
}
