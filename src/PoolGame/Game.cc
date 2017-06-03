#include "Game.hh"
#include <ctime>
#include <iterator>
#include <Windows.h>
#include <algorithm>
#include <vector>

#include "Profiler.hh"

#include "TaskManagerHelpers.hh"
#include <glm/gtc/matrix_transform.inl>

namespace Game
{
	struct GameData
	{
		// Constants
		static constexpr auto GameObjectInvMass = 1.f / 50.f;
		static constexpr auto GameObjectTotalInvMass = GameObjectInvMass * 2.f;
		
		struct GameObjectList
		{
			float posX[MaxGameObjects];
			float posY[MaxGameObjects];
			float velX[MaxGameObjects];
			float velY[MaxGameObjects];

			// Get extreme functions
			constexpr float getMinX(unsigned i) { return posX[i] - GameObjectScale; }
			constexpr float getMaxX(unsigned i) { return posX[i] + GameObjectScale; }
		}
		gameObjects;
		
		// EXTREMES
		static constexpr auto ExtremesSize = MaxGameObjects * 2u;
		struct Extreme 
		{
			float val;
			unsigned index;
			bool min;
			friend inline bool operator <(const Extreme & lhs, const Extreme & rhs) { return (lhs.val < rhs.val); }
		} 
		extremes[ExtremesSize];

		struct ContactData
		{
			unsigned a, b;
			//fVec2 normal;
			float normalX;
			float normalY;
			float penetatrion;
			//GameObjectData pointX;
			//GameObjectData pointY;
			//float totalInvMass; // GameObjectInvMass*2
			//float restitution, friction;
		};

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

		const int screenWidth = input.windowHalfSize.x * 2;
		const int screenHeight = input.windowHalfSize.y * 2;
		const auto aspectRatio = float(screenWidth) / float(screenHeight);
		const int maxCols = pow(MaxGameObjects, 1/aspectRatio);
		const int maxRows = MaxGameObjects / maxCols;
		const auto initX = -input.windowHalfSize.x + ((float)screenWidth / (float)maxCols)/2.f;
		const auto initY = -input.windowHalfSize.y + ((float)screenHeight / (float)maxRows)/2.f;
		/*for (auto i = 0u; i < MaxGameObjects; ++i)
		{
			gameData->gameObjects.posX[i] = initX + (float(i % maxCols) * ((float)screenWidth / (float)maxCols));
			gameData->gameObjects.posY[i] = initY + (float(i / maxCols) * ((float)screenHeight / (float)maxRows - 1));
			gameData->gameObjects.velX[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
			gameData->gameObjects.velY[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
		}*/
		
		/*for (auto i = 0u; i < MaxGameObjects; ++i)
		{
			gameData->gameObjects.posX[i] = rand() % int(input.windowHalfSize.x * 2 - GameObjectScale*2) + -input.windowHalfSize.x+ GameObjectScale * 2;
			gameData->gameObjects.posY[i] = rand() % int(input.windowHalfSize.y * 2 - GameObjectScale*2) + -input.windowHalfSize.y+ GameObjectScale * 2;
			gameData->gameObjects.velX[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
			gameData->gameObjects.velY[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
		}
*/
		for (auto i = 0u; i < MaxGameObjects; ++i)
		{
			gameData->gameObjects.posX[i] = int(i*GameObjectScale * 2) % int(input.windowHalfSize.x * 2 - GameObjectScale * 2) +
				-input.windowHalfSize.x + GameObjectScale * 2;
			gameData->gameObjects.posY[i] = ((int(i*GameObjectScale * 100) / int(input.windowHalfSize.x * 2 - GameObjectScale * 2))) %
				int(input.windowHalfSize.y * 2 - GameObjectScale * 2) + -input.windowHalfSize.y + GameObjectScale * 2;
			gameData->gameObjects.velX[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
			gameData->gameObjects.velY[i] = (rand() % 100 + 50) * (1 - round(float(rand()) / RAND_MAX) * 2.0);
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

	constexpr InputData::ButtonState InputData::ProcessKey(const bool & prevKey, const bool & nowKey)
	{
		if (!prevKey && nowKey)
			return ButtonState::DOWN;

		if (prevKey && !nowKey)
			return ButtonState::UP;

		if (prevKey && nowKey)
			return ButtonState::HOLD;

		return ButtonState::NONE;
	}

	inline void UpdateGameObjects(GameData::GameObjectList &gameObjects_, RenderData & renderData,
								  const InputData& inputData, const Utilities::TaskManager::JobContext &context)
	{
		auto job = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameObjects_, &renderData, &inputData](int i, const Utilities::TaskManager::JobContext& context)
			{
			static constexpr float k0 = 0.99/*0.8*/, k1 = 0.1, k2 = 0.01;

			auto &posX = gameObjects_.posX[i];
			auto &posY = gameObjects_.posY[i];
			auto &velX = gameObjects_.velX[i];
			auto &velY = gameObjects_.velY[i];

			// COMPUTE FRICTION
			fVec2 friction;
			const float brakeValue{ pow(k0, inputData.dt) };
			const float speed{ length(velX, velY) };
			if (speed > 0)
			{
				const float fv{ k1 * speed + k2 * speed * speed };
				friction.x -= fv * (velX / speed);
				friction.y -= fv * (velY / speed);
			}
			const fVec2 acceleration { friction * GameData::GameObjectInvMass };

			// COMPUTE POSITION & VELOCITY
			const auto kdt = (inputData.dt * inputData.dt) / 2.0;
			posX += velX * inputData.dt + acceleration.x * kdt;
			posY += velY * inputData.dt + acceleration.y * kdt;
			velX = brakeValue * velX + acceleration.x * inputData.dt;
			velY = brakeValue * velY + acceleration.y * inputData.dt;

			if (length(velX, velY) < 1e-3) // 1e-1
				velX = velY = 0;

			// CHECK & CORRECT MAP LIMITS
			if (posX > inputData.windowHalfSize.x - GameObjectScale)
				posX = inputData.windowHalfSize.x - GameObjectScale, velX = -velX;
			else if (posX < -inputData.windowHalfSize.x + GameObjectScale)
				posX = -inputData.windowHalfSize.x + GameObjectScale, velX = -velX;

			if (posY > inputData.windowHalfSize.y - GameObjectScale)
				posY = inputData.windowHalfSize.y - GameObjectScale, velY = -velY;
			else if (posY < -inputData.windowHalfSize.y + GameObjectScale)
				posY = -inputData.windowHalfSize.y + GameObjectScale, velY = -velY;

			renderData.colors[i] = { 1, 1, 1, 1 };
			},
			"Update Positions",
			MaxGameObjects / (Utilities::Profiler::MaxNumThreads - 1),
			MaxGameObjects);

		context.DoAndWait(&job);
	}

	constexpr bool HasCollision(GameData::GameObjectList & gameObjects, unsigned indexA, unsigned indexB)
	{
		return distance(gameObjects.posX[indexA], gameObjects.posY[indexA],
						gameObjects.posX[indexB], gameObjects.posY[indexB]) < GameObjectScale + GameObjectScale;
	}

	constexpr GameData::ContactData GenerateContactData(GameData::GameObjectList & gameObjects, unsigned indexA, unsigned indexB)
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
			(GameObjectScale + GameObjectScale) - length(difX, difY), // penetration
		};
	}

	inline void GenerateContactGroups(GameData *& gameData, int createdGroups[Utilities::Profiler::MaxNumThreads][MaxGameObjects],
									  const GameData::ContactData & contact, const Utilities::TaskManager::JobContext & context)
	{
		auto it = createdGroups[context.threadIndex][contact.a]; // busquem si ja tenim alguna colisió amb l'objecte "a"
		if (it == -1)
		{
			it = createdGroups[context.threadIndex][contact.b]; // busquem si ja tenim alguna colisió amb l'objecte "b"
			if (it == -1)
			{
				if (gameData->contactGroupsSizes[context.threadIndex] < gameData->contactGroups[context.threadIndex].size())
				{
					gameData->contactGroups[context.threadIndex][gameData->contactGroupsSizes[context.threadIndex]++] = std::move(GameData::ContactGroup{
						{ contact.a , contact.b }, // game object indexes
						{ contact } // ContactData
					}); // creem la llista de colisions nova
				}
				else
				{
					gameData->contactGroups[context.threadIndex].emplace_back(GameData::ContactGroup{
						{ contact.a , contact.b }, // game object indexes
						{ contact } // ContactData
					}); // creem la llista de colisions nova
				}

				createdGroups[context.threadIndex][contact.a] = gameData->contactGroups[context.threadIndex].size() - 1; // guardem referència a aquesta llista
				createdGroups[context.threadIndex][contact.b] = gameData->contactGroups[context.threadIndex].size() - 1; // per cada objecte per trobarla
			}
			else
			{
				gameData->contactGroups[context.threadIndex][it].contacts.push_back(contact); // afegim la colisió a la llista
				gameData->contactGroups[context.threadIndex][it].objectIndexes.push_back(contact.a);
				createdGroups[context.threadIndex][contact.a] = it; // guardem referència de l'objecte que no hem trobat abans
			}
		}
		else
		{
			auto &groupA = it;
			gameData->contactGroups[context.threadIndex][groupA].contacts.push_back(contact);

			auto itB = createdGroups[context.threadIndex][contact.b];
			if (itB == -1)
			{
				gameData->contactGroups[context.threadIndex][groupA].objectIndexes.push_back(contact.b);
				createdGroups[context.threadIndex][contact.b] = groupA;
			}
			else
			{
				auto &groupB = itB;

				if (groupA != groupB)
				{
					// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

					// 1 - copiem tot el grup anterior
					for (auto &cnt : gameData->contactGroups[context.threadIndex][groupB].contacts)
						gameData->contactGroups[context.threadIndex][groupA].contacts.push_back(cnt);

					// 2 - copiem els elements del segon grup i actualitzem el mapa de grups
					for (auto &index : gameData->contactGroups[context.threadIndex][groupB].objectIndexes)
					{
						if (index != contact.a)
							gameData->contactGroups[context.threadIndex][groupA].objectIndexes.push_back(index);
						createdGroups[context.threadIndex][index] = groupA;
					}

					// 3 - marquem el grup com a buit
					gameData->contactGroups[context.threadIndex][groupB].objectIndexes.clear();
					gameData->contactGroups[context.threadIndex][groupB].contacts.clear();
				}
			}
		}
	}

	// Broad-Phase: "Sort & Sweep"
	//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
	//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
	//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
	//        aquests son les possibles colisions.
	inline void GenerateCollisionGroups(GameData *& gameData, RenderData & renderData, const Utilities::TaskManager::JobContext &context)
	{
		auto jobExtremes = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData](unsigned i, const Utilities::TaskManager::JobContext& context)
			{
				gameData->extremes[i * 2 + 0] = { gameData->gameObjects.getMinX(i), i, true };
				gameData->extremes[i * 2 + 1] = { gameData->gameObjects.getMaxX(i), i, false };
			},
			"Generate Extremes",
			MaxGameObjects / (Utilities::Profiler::MaxNumThreads - 1),
			MaxGameObjects);
		context.DoAndWait(&jobExtremes);

		static constexpr auto NumMergeGroups = 8u; // IMPORTANT: ONLY POWER OF TWO
		std::atomic_int groupCounter = 0;
		auto sortJob = Utilities::TaskManager::CreateLambdaJob(
			[&gameData, &groupCounter](int i, const Utilities::TaskManager::JobContext& context)
		{
			const int index = groupCounter++;
			const int first = GameData::ExtremesSize * (float(index) / float(NumMergeGroups));
			const int last = GameData::ExtremesSize * (float(index + 1) / float(NumMergeGroups));
			std::sort(gameData->extremes + first, gameData->extremes + last);
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
			auto mergeJob = Utilities::TaskManager::CreateLambdaJob(
				[&gameData, &groupCounter, &mergeDivisions, &copiedExtremes](int i, const Utilities::TaskManager::JobContext& context)
			{
				const int index = groupCounter++;
				const int first = GameData::ExtremesSize * (float(index) / float(mergeDivisions));
				const int middle = first + float(GameData::ExtremesSize / mergeDivisions) / 2.0;
				const int last = GameData::ExtremesSize * (float(index + 1) / float(mergeDivisions));
				std::copy(gameData->extremes + first, gameData->extremes + last, copiedExtremes + first);
				std::merge(copiedExtremes + first,
						   copiedExtremes + middle,
						   copiedExtremes + middle,
						   copiedExtremes + last,
						   gameData->extremes + first);
			},
				"Merge Extremes",
				mergeDivisions
				);
			context.DoAndWait(&mergeJob);
		}
		/*context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Sort");
		std::sort(gameData.extremes, gameData.extremes + GameData::ExtremesSize,
				   [](const GameData::Extreme & lhs, const GameData::Extreme & rhs) { return (lhs.val < rhs.val); });
		context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Sort");*/

		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Declarate Groups");
		int createdGroups[Utilities::Profiler::MaxNumThreads][MaxGameObjects];
		for (auto &group : createdGroups)
			for (auto &x : group)
				x = -1;
		std::fill(gameData->contactGroupsSizes, gameData->contactGroupsSizes + Utilities::Profiler::MaxNumThreads, 0);
		context.AddProfileMark(Utilities::Profiler::MarkerType::END_FUNCTION, nullptr, "Declarate Groups");

		auto jobSFG = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&gameData, &renderData, &createdGroups](int i, const Utilities::TaskManager::JobContext& context)
			{
				if (gameData->extremes[i].min)
				{
					for (int j = i + 1; j < GameData::ExtremesSize && gameData->extremes[i].index != gameData->extremes[j].index; ++j)
					{
						if (gameData->extremes[j].min && HasCollision(gameData->gameObjects, gameData->extremes[i].index, gameData->extremes[j].index))
						{
							renderData.colors[gameData->extremes[i].index] = { 2, 0, 2, 1 };
							renderData.colors[gameData->extremes[j].index] = { 0, 2, 2, 1 };
							GenerateContactGroups(gameData,
												  createdGroups,
												  GenerateContactData(gameData->gameObjects, gameData->extremes[i].index, gameData->extremes[j].index),
												  context);
						}
					}
				}
			},
			"Sweep + Fine-Grained + Collision Groups",
			GameData::ExtremesSize / (GameData::ExtremesSize < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
			GameData::ExtremesSize
		);
		context.DoAndWait(&jobSFG);
	}

	constexpr void SolveVelocity(GameData::GameObjectList & gameObjects, GameData::ContactData & contactData)
	{
		const auto &posXA = gameObjects.posX[contactData.a];
		const auto &posYA = gameObjects.posY[contactData.a];
		auto &velXA = gameObjects.velX[contactData.a];
		auto &velYA = gameObjects.velY[contactData.a];
		const auto &posXB = gameObjects.posX[contactData.b];
		const auto &posYB = gameObjects.posY[contactData.b];
		auto &velXB = gameObjects.velX[contactData.b];
		auto &velYB = gameObjects.velY[contactData.b];

		//float approachVel = glm::dot(contactData->a - contactData->b->vel, glm::normalize(contactData->a->pos - contactData->b->pos));
		const auto difVelX = velXA - velXB;
		const auto difVelY = velYA - velYB;
		const float separationVel{ dot(difVelX, difVelY, contactData.normalX, contactData.normalY) };
		
		if (separationVel > 0.0)
		{
			constexpr auto k = 0.7; // 0 - 1 restitution
			const auto impulse{ (-k * separationVel - separationVel) / GameData::GameObjectTotalInvMass };
			const float impulsePerIMass[] {
				contactData.normalX * impulse,
				contactData.normalY * impulse
			};
			velXA += impulsePerIMass[0] * GameData::GameObjectInvMass;
			velYA += impulsePerIMass[1] * GameData::GameObjectInvMass;
			velXB -= impulsePerIMass[0] * GameData::GameObjectInvMass;
			velYB -= impulsePerIMass[1] * GameData::GameObjectInvMass;
		}
	}

	constexpr void SolvePenetration(GameData::GameObjectList & gameObjects, GameData::ContactData & contactData)
	{
		if (contactData.penetatrion > 0.0)
		{
			const float movePerIMass[]{
				contactData.normalX * (contactData.penetatrion / GameData::GameObjectTotalInvMass),
				contactData.normalY * (contactData.penetatrion / GameData::GameObjectTotalInvMass)
			};
			gameObjects.posX[contactData.a] -= movePerIMass[0] * GameData::GameObjectInvMass;
			gameObjects.posY[contactData.a] -= movePerIMass[1] * GameData::GameObjectInvMass;
			gameObjects.posX[contactData.b] += movePerIMass[0] * GameData::GameObjectInvMass;
			gameObjects.posY[contactData.b] += movePerIMass[1] * GameData::GameObjectInvMass;
		}
	}

	void SolveCollisionGroups(GameData *& gameData, const Utilities::TaskManager::JobContext &context)
	{
		context.AddProfileMark(Utilities::Profiler::MarkerType::BEGIN_FUNCTION, nullptr, "Merge Contact Groups");
		std::vector<GameData::ContactGroup> mergedContactGroups;
		for (int i = 0; i < Utilities::Profiler::MaxNumThreads; ++i)
			if (gameData->contactGroupsSizes[i])
				mergedContactGroups.insert(mergedContactGroups.end(), 
										   gameData->contactGroups[i].begin(), 
										   gameData->contactGroups[i].begin() + gameData->contactGroupsSizes[i]);
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

					if (contacts[maxPenetrationIndex].penetatrion < 1e-5)
						break;

					SolveVelocity(gameData->gameObjects, contacts[maxPenetrationIndex]);
					SolvePenetration(gameData->gameObjects, contacts[maxPenetrationIndex]);

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
							if (HasCollision(gameData->gameObjects, mergedContactGroups[i].objectIndexes[j], mergedContactGroups[i].objectIndexes[k]))
							{
								contacts.push_back(GenerateContactData(gameData->gameObjects,
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

	inline void FillRenderData(RenderData & renderData_, GameData *& gameData, const Utilities::TaskManager::JobContext &context)
	{
		auto job = Utilities::TaskManager::CreateLambdaBatchedJob(
			[&renderData_, &gameData](int i, const Utilities::TaskManager::JobContext& context)
		{
			// TODO: optimize
			renderData_.modelMatrices[i] =
				glm::scale(
					glm::translate(glm::mat4(), glm::vec3(gameData->gameObjects.posX[i], gameData->gameObjects.posY[i], 0.f)),
					glm::vec3(Game::GameObjectScale, Game::GameObjectScale, 1.f)
				);
		},
			"Fill Render Data",
			MaxGameObjects / (MaxGameObjects < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads - 1),
			MaxGameObjects);
		context.DoAndWait(&job);
	}

	void Update(RenderData & renderData_, GameData *& gameData, const InputData& inputData, const Utilities::TaskManager::JobContext &context)
	{
		// UPDATE PHYSICS
		{
			auto guard = context.CreateProfileMarkGuard("Update Physics");

			// 1 - Update posicions / velocitats
			UpdateGameObjects(gameData->gameObjects, renderData_, inputData, context);

			// 2 - Generació de colisions
			GenerateCollisionGroups(gameData, renderData_, context);

			// 3 - Resolució de colisions
			SolveCollisionGroups(gameData, context);
		}

		// FILL RENDER
		FillRenderData(renderData_, gameData, context);
	}
	
}
