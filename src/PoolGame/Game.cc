#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "Profiler.hh"

#include "TaskManagerHelpers.hh"
#include <stack>

namespace Game
{
	struct GameData
	{
		// Constants
		static constexpr double GameObjectScale = 3.0;
		static constexpr double GameObjectInvMass = 1.0 / 50.0;
		static constexpr double GameObjectTotalInvMass = GameObjectInvMass * 2;
		static constexpr unsigned MaxGameObjects = 1'000u;
		
		struct GameObjectList
		{
			double posX[MaxGameObjects];
			double posY[MaxGameObjects];
			double velX[MaxGameObjects];
			double velY[MaxGameObjects];

			// Get extreme functions
			inline constexpr double getMinX(unsigned i)	{ return posX[i] - GameObjectScale; }
			inline constexpr double getMaxX(unsigned i) { return posX[i] + GameObjectScale; }
		}
		gameObjects;
		
		// EXTREMES
		static constexpr auto ExtremesSize = MaxGameObjects * 2u;
		struct /*alignas(16)*/ Extreme 
		{
			double val;
			unsigned index;
			bool min;
		} 
		extremes[ExtremesSize];

		// POSSIBLE COLLISIONS
		struct PossibleCollission { unsigned a, b; };
		std::vector<PossibleCollission> possibleCollisions[Utilities::Profiler::MaxNumThreads];
		unsigned possibleCollisionsSize[Utilities::Profiler::MaxNumThreads];

		struct ContactData
		{
			unsigned a, b;
			double normalX;
			double normalY;
			//GameObjectData pointX;
			//GameObjectData pointY;
			double penetatrion;
			// double totalInvMass; // GameObjectInvMass*2
			//double restitution, friction;
		};
		std::vector<ContactData> contactDataList;

		struct ContactGroup
		{
			std::vector<unsigned> objectIndexes;
			std::vector<ContactData> contacts;
		};
		std::vector<ContactGroup> contactGroups;
	};

	GameData* InitGamedata(const InputData & input)
	{
		srand(static_cast<unsigned>(time(nullptr)));

		GameData * gameData = new GameData;

		const auto initX = int(- input.windowHalfSize.x + GameData::GameObjectScale);
		const auto initY = int(- input.windowHalfSize.y + GameData::GameObjectScale);
		const auto maxCols = (input.windowHalfSize.x * 2) / (GameData::GameObjectScale * 4);
		const auto maxRows = (input.windowHalfSize.y * 2) / (GameData::GameObjectScale * 4);
		for (auto i = 0u; i < GameData::MaxGameObjects; ++i)
		{
			//gameData->gameObjects.posX[i] = (i % int(maxCols)) * GameData::GameObjectScale * 4 - initX;
			//gameData->gameObjects.posY[i] = int(i / (maxCols)) * GameData::GameObjectScale * 4 + initY;
			gameData->gameObjects.posX[i] = rand() % int(input.windowHalfSize.x * 2 - GameData::GameObjectScale*2) + initX;
			gameData->gameObjects.posY[i] = rand() % int(input.windowHalfSize.y * 2 - GameData::GameObjectScale*2) + initY;
			gameData->gameObjects.velX[i] = (rand() % 200 + 50) * (1 - round(double(rand()) / RAND_MAX) * 2.0);
			gameData->gameObjects.velY[i] = (rand() % 200 + 50) * (1 - round(double(rand()) / RAND_MAX) * 2.0);
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

	template<class T>
	inline constexpr int sgn(T val)
	{
		return val < T(0) ? -1 : 1;
	}

	template<class T>
	inline constexpr T dot(const T & xA, const T & yA, const T & xB, const T & yB)
	{
		return xA*xB + yA*yB;
	}

	template<class T>
	inline constexpr T length(const T & x, const T & y)
	{
		static_assert(std::numeric_limits<T>::is_iec559, "'length' accepts only floating-point inputs");
		return sqrt(dot(x, y, x, y));
	}

	template<class T>
	inline constexpr T distance(const T & xA, const T & yA, const T & xB, const T & yB)
	{
		return length(xB - xA, yB - yA);
	}

	template<class T>
	inline constexpr T normalizeX(const T & xA, const T & yA)
	{
		return xA / length(xA, yA);
	}

	template<class T>
	inline constexpr T normalizeY(const T & xA, const T & yA)
	{
		return yA / length(xA, yA);
	}

	template <class T>
	inline constexpr T normalize(const T & x)
	{
		static_assert(std::numeric_limits<T>::is_iec559, "'normalize' accepts only floating-point inputs");
		return x < T(0) ? T(-1) : T(1);
	}

	inline void UpdateGameObjects(GameData::GameObjectList &gameObjects_, const InputData& inputData, const Utilities::TaskManager::JobContext &context)
	{
		auto job = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameObjects_, &inputData](int i, const Utilities::TaskManager::JobContext& context)
			{
			static constexpr double k0 = 0.99/*0.8*/, k1 = 0.1, k2 = 0.01;

			auto &posX = gameObjects_.posX[i];
			auto &posY = gameObjects_.posY[i];
			auto &velX = gameObjects_.velX[i];
			auto &velY = gameObjects_.velY[i];

			// COMPUTE FRICTION
			double frictionX = 0, frictionY = 0;
			const double brakeValue{ pow(k0, inputData.dt) };
			const double speed{ length(velX, velY) };
			if (speed > 0)
			{
				const double fv{ k1 * speed + k2 * speed * speed };
				frictionX -= fv * (velX / speed);
				frictionY -= fv * (velY / speed);
			}
			const double accelerationX{ frictionX * GameData::GameObjectInvMass };
			const double accelerationY{ frictionY * GameData::GameObjectInvMass };

			// COMPUTE POSITION & VELOCITY
			const auto kdt = (inputData.dt * inputData.dt) / 2.0;
			const auto prevPosX = posX, prevPosY = posY;
			posX += velX * inputData.dt + accelerationX * kdt;
			posY += velY * inputData.dt + accelerationY * kdt;
			velX = brakeValue * velX + accelerationX * inputData.dt;
			velY = brakeValue * velY + accelerationY * inputData.dt;

			if (length(velX, velY) < 1e-3) //1e-1
				velX = velY = 0;

			// CHECK & CORRECT MAP LIMITS
			if (posX > inputData.windowHalfSize.x - GameData::GameObjectScale)
				posX = inputData.windowHalfSize.x - GameData::GameObjectScale, velX = -velX;
			else if (posX < -inputData.windowHalfSize.x + GameData::GameObjectScale)
				posX = -inputData.windowHalfSize.x + GameData::GameObjectScale, velX = -velX;

			if (posY > inputData.windowHalfSize.y - GameData::GameObjectScale)
				posY = inputData.windowHalfSize.y - GameData::GameObjectScale, velY = -velY;
			else if (posY < -inputData.windowHalfSize.y + GameData::GameObjectScale)
				posY = -inputData.windowHalfSize.y + GameData::GameObjectScale, velY = -velY;
			},
			"Update Positions",
			GameData::MaxGameObjects / (GameData::MaxGameObjects < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			GameData::MaxGameObjects);

		context.DoAndWait(&job);
	}

	// Broad-Phase: "Sort & Sweep"
	//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
	//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
	//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
	//        aquests son les possibles colisions.
	void SortAndSweep(GameData & gameData, const Utilities::TaskManager::JobContext &context)
	{
		auto jobA = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](unsigned i, const Utilities::TaskManager::JobContext& context)
			{
				gameData.extremes[i * 2 + 0] = { gameData.gameObjects.getMinX(i), i, true };
				gameData.extremes[i * 2 + 1] = { gameData.gameObjects.getMaxX(i), i, false };
			},
			"Sort & Sweep create list",
			GameData::MaxGameObjects / Utilities::Profiler::MaxNumThreads,
			GameData::MaxGameObjects);
		context.DoAndWait(&jobA);

		/*auto jobSort = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](int i, const Utilities::TaskManager::JobContext& context)
			{
				std::sort(gameData.extremes + GameData::ExtremesSize * ((context.threadIndex-1) / Utilities::Profiler::MaxNumThreads - 1),
						  gameData.extremes + GameData::ExtremesSize * (context.threadIndex / Utilities::Profiler::MaxNumThreads - 1), 
						  [](const GameData::Extreme & lhs, const GameData::Extreme & rhs) { return (lhs.val < rhs.val); });
			},
			"Sort",
			GameData::ExtremesSize / (GameData::ExtremesSize < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			GameData::ExtremesSize
		);
		context.DoAndWait(&jobSort);*/
		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Sort");
		std::sort(gameData.extremes, gameData.extremes + GameData::ExtremesSize, 
				  [](const GameData::Extreme & lhs, const GameData::Extreme & rhs) { return (lhs.val < rhs.val); });
		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Sort");

		
		std::fill(gameData.possibleCollisionsSize, gameData.possibleCollisionsSize + Utilities::Profiler::MaxNumThreads, 0);

		auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](int i, const Utilities::TaskManager::JobContext& context)
			{
				if (gameData.extremes[i].min)
					for (int j = i + 1; j < GameData::ExtremesSize && gameData.extremes[i].index != gameData.extremes[j].index; ++j)
						if (gameData.extremes[j].min)
						{
							if (gameData.possibleCollisionsSize[context.threadIndex] < gameData.possibleCollisions[context.threadIndex].size())
								gameData.possibleCollisions[context.threadIndex][gameData.possibleCollisionsSize[context.threadIndex]++] = { gameData.extremes[i].index, gameData.extremes[j].index };
							else
								gameData.possibleCollisions[context.threadIndex].push_back({ gameData.extremes[i].index, gameData.extremes[j].index });
						}
			},
			"Sweep",
			GameData::ExtremesSize / (GameData::ExtremesSize < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			GameData::ExtremesSize
		);
		context.DoAndWait(&jobB);

		//gameData.realPossibleCollisions.reserve(gameData.possibleCollisionsRealSize); //memmove

		//context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Real Possible Collisions");
		//gameData.realPossibleCollisions.clear();
		//for (auto i = 0u; i < Utilities::Profiler::MaxNumThreads; ++i) // TODO: optimize
		//{
		//	gameData.realPossibleCollisions.insert(gameData.realPossibleCollisions.end(), gameData.possibleCollisions[i].begin(), gameData.possibleCollisions[i].end());
		//}
		//context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Real Possible Collisions");
	}

	inline constexpr bool HasCollision(GameData::GameObjectList & gameObjects, unsigned indexA, unsigned indexB)
	{
		return distance(gameObjects.posX[indexA], gameObjects.posY[indexA], 
						gameObjects.posX[indexB], gameObjects.posY[indexB]) < GameData::GameObjectScale + GameData::GameObjectScale;
	}

	inline constexpr GameData::ContactData GenerateContactData(GameData::GameObjectList & gameObjects, unsigned indexA, unsigned indexB)
	{
		const auto &posXA = gameObjects.posX[indexA];
		const auto &posYA = gameObjects.posY[indexA];
		const auto &posXB = gameObjects.posX[indexB];
		const auto &posYB = gameObjects.posY[indexB];
		const auto difX = posXB - posXA;
		const auto difY = posYB - posYA;

		//posXA + normal[0] * GameData::GameObjectScale, posXA + normal[1] * GameData::GameObjectScale, // point
		return GameData::ContactData {
			indexA, // a
			indexB, // b
			normalizeX(difX, difY), normalizeY(difX, difY), // normal
			(GameData::GameObjectScale + GameData::GameObjectScale) - length(difX, difY), // penetration
		};
	}

	inline void FineGrained(GameData &gameData, const Utilities::TaskManager::JobContext &context)
	{
		auto size = 0u;
		for (auto &pcs : gameData.possibleCollisionsSize) size += pcs;
		gameData.contactDataList.resize(size); // TODO: resize ?

		auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](int j, const Utilities::TaskManager::JobContext& context)
		{
			if (HasCollision(gameData.gameObjects, gameData.possibleCollisions[context.threadIndex][j].a, gameData.possibleCollisions[context.threadIndex][j].b))
				gameData.contactDataList[contextA.threadIndex][j] = GenerateContactData(gameData.gameObjects,
																						gameData.possibleCollisions[context.threadIndex][j].a,
																						gameData.possibleCollisions[context.threadIndex][j].b);
		},
			"Generate Contact Data",
			gameData.possibleCollisions[contextA.threadIndex].size() /
			(gameData.possibleCollisions[contextA.threadIndex].size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			gameData.possibleCollisions[contextA.threadIndex].size());

		contextA.DoAndWait(&jobB);
	}

	inline void GenerateContactGroups(GameData & gameData, const Utilities::TaskManager::JobContext &context)
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

		
		std::unordered_map<unsigned, GameData::ContactGroup*> createdGroups[Utilities::Profiler::MaxNumThreads];

		auto jobA = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData, &createdGroups](int k, const Utilities::TaskManager::JobContext& contextA)
		{
			gameData.contactGroups[contextA.threadIndex].reserve(gameData.contactDataList[contextA.threadIndex].size());

			auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
				[&gameData, &createdGroups, &contextA](int i, const Utilities::TaskManager::JobContext& contextB)
			{
				auto it = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].a); // busquem si ja tenim alguna colisió amb l'objecte "a"
				if (it == createdGroups[contextA.threadIndex].end())
				{
					it = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].b); // busquem si ja tenim alguna colisió amb l'objecte "b"
					if (it == createdGroups[contextA.threadIndex].end())
					{
						gameData.contactGroups[contextA.threadIndex].push_back(GameData::ContactGroup{
							{ gameData.contactDataList[contextA.threadIndex][i].a , gameData.contactDataList[contextA.threadIndex][i].b }, // game object indexes
							{ gameData.contactDataList[contextA.threadIndex][i] } // ContactData
						}); // creem la llista de colisions nova

						createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].a] = &gameData.contactGroups[contextA.threadIndex]
							[gameData.contactGroups[contextA.threadIndex].size() - 1]; // guardem referència a aquesta llista
						createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].b] = &gameData.contactGroups[contextA.threadIndex]
							[gameData.contactGroups[contextA.threadIndex].size() - 1]; // per cada objecte per trobarla
					}
					else
					{
						it->second->contacts.push_back(gameData.contactDataList[contextA.threadIndex][i]); // afegim la colisió a la llista
						it->second->objectIndexes.push_back(gameData.contactDataList[contextA.threadIndex][i].a);
						createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].a] = it->second; // guardem referència de l'objecte que no hem trobat abans
					}
				}
				else
				{
					GameData::ContactGroup *groupA = it->second;
					groupA->contacts.push_back(gameData.contactDataList[contextA.threadIndex][i]);

					auto itB = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].b);
					if (itB == createdGroups[contextA.threadIndex].end())
					{
						groupA->objectIndexes.push_back(gameData.contactDataList[contextA.threadIndex][i].b);
						createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].b] = groupA;
					}
					else
					{
						GameData::ContactGroup *groupB = itB->second;

						if (groupA != groupB)
						{
							// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

							// 1 - copiem tot el grup anterior
							for (auto& contact : groupB->contacts)
							{
								groupA->contacts.push_back(contact);
							}

							// 2 - copiem els elements del segon grup i actualitzem el mapa
							//     de grups
							for (auto &index : groupB->objectIndexes)
							{
								if (index != gameData.contactDataList[contextA.threadIndex][i].a)
									groupA->objectIndexes.push_back(index);
								createdGroups[contextA.threadIndex][index] = groupA;
							}

							// 3 - marquem el grup com a buit
							groupB->objectIndexes.clear();
							groupB->contacts.clear();
						}
					}
				}
			},
			"Generate Contact Groups",
			Utilities::Profiler::MaxNumThreads,
			Utilities::Profiler::MaxNumThreads);

			contextA.DoAndWait(&jobB);
		},
		"Generate Contact Groups List",
		Utilities::Profiler::MaxNumThreads,
		Utilities::Profiler::MaxNumThreads);
		context.DoAndWait(&jobA);
	}

	inline constexpr void SolveVelocity(GameData::GameObjectList & gameObjects, GameData::ContactData & contactData)
	{
		const auto &posXA = gameObjects.posX[contactData.a];
		const auto &posYA = gameObjects.posY[contactData.a];
		auto &velXA = gameObjects.velX[contactData.a];
		auto &velYA = gameObjects.velY[contactData.a];
		const auto &posXB = gameObjects.posX[contactData.b];
		const auto &posYB = gameObjects.posY[contactData.b];
		auto &velXB = gameObjects.velX[contactData.b];
		auto &velYB = gameObjects.velY[contactData.b];

		//double approachVel = glm::dot(contactData->a - contactData->b->vel, glm::normalize(contactData->a->pos - contactData->b->pos));
		const auto difVelX = velXA - velXB;
		const auto difVelY = velYA - velYB;
		const double separationVel{ dot(difVelX, difVelY, contactData.normalX, contactData.normalY) };
		
		if (separationVel > 0.0)
		{
			constexpr auto k = 0.7; // 0 - 1 restitution
			const auto impulse{ (-k * separationVel - separationVel) / GameData::GameObjectTotalInvMass };
			const double impulsePerIMass[] {
				contactData.normalX * impulse,
				contactData.normalY * impulse
			};
			velXA += impulsePerIMass[0] * GameData::GameObjectInvMass;
			velYA += impulsePerIMass[1] * GameData::GameObjectInvMass;
			velXB -= impulsePerIMass[0] * GameData::GameObjectInvMass;
			velYB -= impulsePerIMass[1] * GameData::GameObjectInvMass;
		}
	}

	inline constexpr void SolvePenetration(GameData::GameObjectList & gameObjects, GameData::ContactData & contactData)
	{
		if (contactData.penetatrion > 0.0)
		{
			const double movePerIMass[]{
				contactData.normalX * (contactData.penetatrion / GameData::GameObjectTotalInvMass),
				contactData.normalY * (contactData.penetatrion / GameData::GameObjectTotalInvMass)
			};
			gameObjects.posX[contactData.a] -= movePerIMass[0] * GameData::GameObjectInvMass;
			gameObjects.posY[contactData.a] -= movePerIMass[1] * GameData::GameObjectInvMass;
			gameObjects.posX[contactData.b] += movePerIMass[0] * GameData::GameObjectInvMass;
			gameObjects.posY[contactData.b] += movePerIMass[1] * GameData::GameObjectInvMass;
		}
	}

	void SolveCollisionGroups(GameData & gameData, const Utilities::TaskManager::JobContext &context)
	{
		for (auto &cgs : gameData.contactGroups)
		{
			if (!cgs.empty())
			{
				auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
					[&gameData, &cgs](int i, const Utilities::TaskManager::JobContext& context)
				{
					std::vector<GameData::ContactData> contacts{ gameData.contactGroups[context.threadIndex][i].contacts };
					int iterations = 0;
					while (contacts.size() > 0 && iterations < gameData.contactGroups[context.threadIndex][i].contacts.size() * 3)
					{
						// busquem la penetració més gran
						int MaxPenetrationIndex = 0;
						for (int j = 0; j < contacts.size(); j++)
							if (contacts[j].penetatrion >= contacts[MaxPenetrationIndex].penetatrion)
								MaxPenetrationIndex = j;

						if (contacts[MaxPenetrationIndex].penetatrion < 1e-5)
							break;

						SolveVelocity(gameData.gameObjects, contacts[MaxPenetrationIndex]);
						SolvePenetration(gameData.gameObjects, contacts[MaxPenetrationIndex]);

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
						for (int j = 0; j < gameData.contactGroups[context.threadIndex][i].objectIndexes.size(); ++j)
						{
							for (int k = j + 1; k < gameData.contactGroups[context.threadIndex][i].objectIndexes.size(); ++k)
							{
								if (HasCollision(gameData.gameObjects, gameData.contactGroups[context.threadIndex][i].objectIndexes[j], gameData.contactGroups[context.threadIndex][i].objectIndexes[k]))
								{
									contacts.push_back(GenerateContactData(gameData.gameObjects, gameData.contactGroups[context.threadIndex][i].objectIndexes[k], gameData.contactGroups[contextA.threadIndex][i].objectIndexes[j]));
								}
							}
						}
						#endif
						++iterations;
					}
				},
					"Group Solver",
					gameData.contactGroups[contextA.threadIndex].size()
					/ (gameData.contactGroups[contextA.threadIndex].size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
					gameData.contactGroups[contextA.threadIndex].size());
				contextA.DoAndWait(&jobB);
			}
		}
	}

	RenderData Update(GameData & gameData, const InputData& inputData, const Utilities::TaskManager::JobContext &context)
	{
		// UPDATE PHYSICS
		{
			auto guard = context.CreateProfileMarkGuard("Update Physics");

			// 1 - Update posicions / velocitats
			UpdateGameObjects(gameData.gameObjects, inputData, context);

			// 2 - Generació de colisions

			//   2.1 - Broad-Phase: "Sort & Sweep"
			SortAndSweep(gameData, context);

			//   2.2 - Fine-Grained
			FineGrained(gameData, context);

			//   2.3 - Sort / Generació de grups de col·lisions
			GenerateContactGroups(gameData, context);

			// 3 - Resolució de colisions
			SolveCollisionGroups(gameData, context);
		}

		// RENDER BALLS
		RenderData renderData {};
		renderData.sprites.resize(GameData::MaxGameObjects);
		auto job = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&renderData, &gameData](int i, const Utilities::TaskManager::JobContext& context)
			{
				renderData.sprites[i].position = { gameData.gameObjects.posX[i], gameData.gameObjects.posY[i] };
				renderData.sprites[i].size = { GameData::GameObjectScale, GameData::GameObjectScale };
				renderData.sprites[i].color = { 1.f, 1.f, 1.f };
			},
			"Fill Render Data",
			GameData::MaxGameObjects / (GameData::MaxGameObjects < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			GameData::MaxGameObjects);
		context.DoAndWait(&job);

		return renderData;
	}
	
}
