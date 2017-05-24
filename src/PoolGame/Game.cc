#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "Profiler.hh"

#include "TaskManagerHelpers.hh"
#include "Utils.inl.hh"

namespace Game
{
	struct GameData
	{
		// Constants
		static constexpr double GameObjectScale = 1.0;
		static constexpr double GameObjectInvMass = 1.0 / 50.0;
		static constexpr double GameObjectTotalInvMass = GameObjectInvMass * 2;
		static constexpr unsigned MaxGameObjects = 15'000u;
		
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
			friend inline bool operator <(const Extreme & lhs, const Extreme & rhs) { return (lhs.val < rhs.val); }
		} 
		extremes[ExtremesSize];

		// POSSIBLE COLLISIONS
		//struct PossibleCollission { unsigned a, b; };
		//std::vector<PossibleCollission> possibleCollisions[Utilities::Profiler::MaxNumThreads];

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
		//std::vector<ContactData> contactDataList[Utilities::Profiler::MaxNumThreads];

		struct ContactGroup
		{
			std::vector<unsigned> objectIndexes;
			std::vector<ContactData> contacts;
		};
		std::vector<ContactGroup> contactGroups[Utilities::Profiler::MaxNumThreads];
		unsigned contactGroupsSizes[Utilities::Profiler::MaxNumThreads];
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
			gameData->gameObjects.velX[i] = (rand() % 100 + 50) * (1 - round(double(rand()) / RAND_MAX) * 2.0);
			gameData->gameObjects.velY[i] = (rand() % 100 + 50) * (1 - round(double(rand()) / RAND_MAX) * 2.0);
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
		return GameData::ContactData{
			indexA, // a
			indexB, // b
			normalizeX(difX, difY), normalizeY(difX, difY), // normal
			(GameData::GameObjectScale + GameData::GameObjectScale) - length(difX, difY), // penetration
		};
	}

	inline void GenerateContactGroups(GameData & gameData, std::unordered_map<unsigned, unsigned> createdGroups[Utilities::Profiler::MaxNumThreads],
									  const GameData::ContactData & contact, const Utilities::TaskManager::JobContext & context)
	{
		auto it = createdGroups[context.threadIndex].find(contact.a); // busquem si ja tenim alguna colisió amb l'objecte "a"
		if (it == createdGroups[context.threadIndex].end())
		{
			it = createdGroups[context.threadIndex].find(contact.b); // busquem si ja tenim alguna colisió amb l'objecte "b"
			if (it == createdGroups[context.threadIndex].end())
			{
				if (gameData.contactGroupsSizes[context.threadIndex] < gameData.contactGroups[context.threadIndex].size())
				{
					gameData.contactGroups[context.threadIndex][gameData.contactGroupsSizes[context.threadIndex]++] = std::move(GameData::ContactGroup{
						{ contact.a , contact.b }, // game object indexes
						{ contact } // ContactData
					}); // creem la llista de colisions nova
				}
				else
				{
					gameData.contactGroups[context.threadIndex].emplace_back(GameData::ContactGroup{
						{ contact.a , contact.b }, // game object indexes
						{ contact } // ContactData
					}); // creem la llista de colisions nova
				}

				createdGroups[context.threadIndex][contact.a] = gameData.contactGroups[context.threadIndex].size() - 1; // guardem referència a aquesta llista
				createdGroups[context.threadIndex][contact.b] = gameData.contactGroups[context.threadIndex].size() - 1; // per cada objecte per trobarla
			}
			else
			{
				gameData.contactGroups[context.threadIndex][it->second].contacts.push_back(contact); // afegim la colisió a la llista
				gameData.contactGroups[context.threadIndex][it->second].objectIndexes.push_back(contact.a);
				createdGroups[context.threadIndex][contact.a] = it->second; // guardem referència de l'objecte que no hem trobat abans
			}
		}
		else
		{
			auto &groupA = it->second;
			gameData.contactGroups[context.threadIndex][groupA].contacts.push_back(contact);

			auto itB = createdGroups[context.threadIndex].find(contact.b);
			if (itB == createdGroups[context.threadIndex].end())
			{
				gameData.contactGroups[context.threadIndex][groupA].objectIndexes.push_back(contact.b);
				createdGroups[context.threadIndex][contact.b] = groupA;
			}
			else
			{
				auto &groupB = itB->second;

				if (groupA != groupB)
				{
					// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

					// 1 - copiem tot el grup anterior
					for (auto& cnt : gameData.contactGroups[context.threadIndex][groupB].contacts)
						gameData.contactGroups[context.threadIndex][groupA].contacts.push_back(cnt);

					// 2 - copiem els elements del segon grup i actualitzem el mapa
					//     de grups
					for (auto &index : gameData.contactGroups[context.threadIndex][groupB].objectIndexes)
					{
						if (index != contact.a)
							gameData.contactGroups[context.threadIndex][groupA].objectIndexes.push_back(index);
						createdGroups[context.threadIndex][index] = groupA;
					}

					// 3 - marquem el grup com a buit
					gameData.contactGroups[context.threadIndex][groupB].objectIndexes.clear();
					gameData.contactGroups[context.threadIndex][groupB].contacts.clear();
				}
			}
		}
	}

	// Broad-Phase: "Sort & Sweep"
	//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
	//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
	//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
	//        aquests son les possibles colisions.
	inline void SortAndSweep(GameData & gameData, const Utilities::TaskManager::JobContext &context)
	{
		auto jobExtremes = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](unsigned i, const Utilities::TaskManager::JobContext& context)
			{
				gameData.extremes[i * 2 + 0] = { gameData.gameObjects.getMinX(i), i, true };
				gameData.extremes[i * 2 + 1] = { gameData.gameObjects.getMaxX(i), i, false };
			},
			"Generate Extremes",
			GameData::MaxGameObjects / Utilities::Profiler::MaxNumThreads,
			GameData::MaxGameObjects);
		context.DoAndWait(&jobExtremes);

		static constexpr auto NumMergeGroups = 32u; // IMPORTANT: ONLY POWER OF TWO
		std::atomic_int groupCounter = 0;
		auto sortJob = Utilities::TaskManager::CreateLambdaJob(
			[&gameData, &groupCounter](int i, const Utilities::TaskManager::JobContext& context)
		{
			const int index = groupCounter++;
			const int first = GameData::ExtremesSize * (double(index) / double(NumMergeGroups));
			const int last = GameData::ExtremesSize * (double(index + 1) / double(NumMergeGroups));
			std::sort(gameData.extremes + first, gameData.extremes + last);
		},
			"Divide & Sort Extremes",
			NumMergeGroups
		);
		context.DoAndWait(&sortJob);

		auto mergeDivisions = NumMergeGroups;
		GameData::Extreme copiedExtremes[GameData::ExtremesSize];
		while (mergeDivisions > 1)
		{
			mergeDivisions /= 2;
			groupCounter = 0;
			std::copy(gameData.extremes, gameData.extremes + GameData::ExtremesSize, copiedExtremes);
			auto mergeJob = Utilities::TaskManager::CreateLambdaJob(
				[&gameData, &groupCounter, &mergeDivisions, &copiedExtremes](int i, const Utilities::TaskManager::JobContext& context)
			{
				const int index = groupCounter++;
				const int first = GameData::ExtremesSize * (double(index) / double(mergeDivisions));
				const int middle = first + double(GameData::ExtremesSize / mergeDivisions) / 2.0;
				const int last = GameData::ExtremesSize * (double(index + 1) / double(mergeDivisions));
				std::merge(copiedExtremes + first,
						   copiedExtremes + middle,
						   copiedExtremes + middle,
						   copiedExtremes + last,
						   gameData.extremes + first);
			},
				"Merge Sort Extremes",
				mergeDivisions
				);
			context.DoAndWait(&mergeJob);
		}
		/*context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Sort");
		std::sort(gameData.extremes, gameData.extremes + GameData::ExtremesSize,
				   [](const GameData::Extreme & lhs, const GameData::Extreme & rhs) { return (lhs.val < rhs.val); });
		context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Sort");*/

		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Declarate Groups");
		std::unordered_map<unsigned, unsigned> createdGroups[Utilities::Profiler::MaxNumThreads];
		std::fill(gameData.contactGroupsSizes, gameData.contactGroupsSizes + Utilities::Profiler::MaxNumThreads, 0);
		context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Declarate Groups");

		auto jobSFG = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData, &createdGroups](int i, const Utilities::TaskManager::JobContext& context)
			{
			//for (int i = 0; i < GameData::ExtremesSize; ++i)
				if (gameData.extremes[i].min)
				{
					for (int j = i + 1; j < GameData::ExtremesSize && gameData.extremes[i].index != gameData.extremes[j].index; ++j)
					{
						if (gameData.extremes[j].min && HasCollision(gameData.gameObjects, gameData.extremes[i].index, gameData.extremes[j].index))
						{
							GenerateContactGroups(gameData,
												  createdGroups,
												  GenerateContactData(gameData.gameObjects, gameData.extremes[i].index, gameData.extremes[j].index),
												  context);
						}
					}
				}
			},
			"Sweep + Fine-Grained + Collision Groups",
			GameData::ExtremesSize / Utilities::Profiler::MaxNumThreads,
			GameData::ExtremesSize
		);
		context.DoAndWait(&jobSFG);
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
		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Merge Contact Groups");
		std::vector<GameData::ContactGroup> mergedContactGroups;
		for (int i = 0; i < Utilities::Profiler::MaxNumThreads; ++i)
			if (gameData.contactGroupsSizes[i])
				mergedContactGroups.insert(mergedContactGroups.end(), gameData.contactGroups[i].begin(), gameData.contactGroups[i].begin() + gameData.contactGroupsSizes[i]);
		context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Merge Contact Groups");

		if (mergedContactGroups.size())
		{
			auto jobA = Utilities::TaskManager::CreateLambdaBatchedJob(
				[&gameData, &mergedContactGroups](int i, const Utilities::TaskManager::JobContext& context)
			{
				std::vector<GameData::ContactData> &contacts{ mergedContactGroups[i].contacts };
				int iterations = 0;
				while (contacts.size() > 0 && iterations < mergedContactGroups[i].contacts.size() * 3)
				{
					// busquem la penetració més gran
					int maxPenetrationIndex{ 0 };
					for (int j = 0; j < contacts.size(); ++j)
						if (contacts[j].penetatrion >= contacts[maxPenetrationIndex].penetatrion)
							maxPenetrationIndex = j;
					/*std::atomic_int maxPenetrationIndex{ 0 };
					auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
						[&maxPenetrationIndex, &contacts](int j, const Utilities::TaskManager::JobContext& context)
						{
						if (contacts[j].penetatrion >= contacts[maxPenetrationIndex.load()].penetatrion)
							maxPenetrationIndex.store(j);
						},
						"Penetration Search",
						contacts.size() / (contacts.size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
						contacts.size());
					context.DoAndWait(&jobB);*/

					if (contacts[maxPenetrationIndex].penetatrion < 1e-5)
						break;

					SolveVelocity(gameData.gameObjects, contacts[maxPenetrationIndex]);
					SolvePenetration(gameData.gameObjects, contacts[maxPenetrationIndex]);

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
					for (int j = 0; j < mergedContactGroups[i].objectIndexes.size(); ++j)
					{
						for (int k = j + 1; k < mergedContactGroups[i].objectIndexes.size(); ++k)
						{
							if (HasCollision(gameData.gameObjects, mergedContactGroups[i].objectIndexes[j], mergedContactGroups[i].objectIndexes[k]))
							{
								contacts.push_back(GenerateContactData(gameData.gameObjects,
												   mergedContactGroups[i].objectIndexes[k],
												   mergedContactGroups[i].objectIndexes[j]));
							}
						}
					}
					/*if (mergedContactGroups[i].objectIndexes.size())
					{
						auto jobC = Utilities::TaskManager::CreateLambdaBatchedJob(
							[&gameData, &mergedContactGroups, &contacts, &i](int j, const Utilities::TaskManager::JobContext& context)
						{
							for (int k = j + 1; k < mergedContactGroups[i].objectIndexes.size(); ++k)
							{
								if (HasCollision(gameData.gameObjects, mergedContactGroups[i].objectIndexes[j], mergedContactGroups[i].objectIndexes[k]))
								{
									contacts.push_back(GenerateContactData(gameData.gameObjects,
													   mergedContactGroups[i].objectIndexes[k],
													   mergedContactGroups[i].objectIndexes[j]));
								}
							}
						},
							"Generate Contacts Again",
							mergedContactGroups[i].objectIndexes.size() /
							(mergedContactGroups[i].objectIndexes.size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
							mergedContactGroups[i].objectIndexes.size());
						context.DoAndWait(&jobC);
					}*/
					#endif
					++iterations;
				}
			},
				"Group Solver",
				(mergedContactGroups.size() / (mergedContactGroups.size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads)),
				mergedContactGroups.size());

			context.DoAndWait(&jobA);
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
			//FineGrained(gameData, context);

			//   2.3 - Sort / Generació de grups de col·lisions
			//GenerateContactGroups(gameData, context);

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
			(GameData::MaxGameObjects / (GameData::MaxGameObjects < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads)),
			GameData::MaxGameObjects);
		context.DoAndWait(&job);

		return renderData;
	}
	
}
