//GameObject &whiteBall;

/*struct TouchInfo {
bool isTouched { false };
glm::dvec2 posReleased {};
glm::dvec2 posTouched {};
} whiteBallTouchInfo;*/

//gameData->whiteBall = gameData->balls[0];

/*int c = 1;
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
gameData->whiteBall->pos.y = 0;*/

//for (int i = static_cast<int>(Game::RenderData::TextureID::BALL_0);
//	 i < static_cast<int>(Game::RenderData::TextureID::MAX_BALLS);
//	 ++i)
//{
//	//ball.vel = glm::dvec2{ rand() % 100 + 50, rand() % 100 + 50 } * (1 - round(double(rand()) / double(RAND_MAX)) * 2.0);
//	gameData->balls[i].scale = 3.0;
//	gameData->balls[i].invMass = 1 / 50.0;
//	gameData->balls[i].col.position = gameData->balls[i].pos;
//	gameData->balls[i].col.radius = gameData->balls[i].scale;
//}

// PROCESS INPUT
//if (inputData.mouseButtonL == InputData::ButtonState::DOWN || inputData.mouseButtonL == InputData::ButtonState::HOLD)
//{
//	auto realMouseX = inputData.mousePosition.x - inputData.windowHalfSize.x;
//	auto realMouseY = inputData.windowHalfSize.y - inputData.mousePosition.y;

//	if (gameData.whiteBall->pos.x - gameData.whiteBall->scale < double(realMouseX) &&
//		gameData.whiteBall->pos.x + gameData.whiteBall->scale > double(realMouseX) &&
//		gameData.whiteBall->pos.y - gameData.whiteBall->scale < double(realMouseY) &&
//		gameData.whiteBall->pos.y + gameData.whiteBall->scale > double(realMouseY))
//	{
//		/*char buffer[512];
//		sprintf_s(buffer, "x: %f, y: %d\nx: %f, y : %d\n\n", gameData.whiteBall->pos.x, realMouseX, gameData.whiteBall->pos.y, realMouseY);
//		OutputDebugStringA(buffer);*/
//		//gameData.whiteBall->vel = {};
//		gameData.whiteBallTouchInfo.isTouched = true;
//		gameData.whiteBallTouchInfo.posTouched = gameData.whiteBall->pos;
//	}
//}
//if (gameData.whiteBallTouchInfo.isTouched)
//{
//	auto realMouseX = inputData.mousePosition.x - inputData.windowHalfSize.x;
//	auto realMouseY = inputData.windowHalfSize.y - inputData.mousePosition.y;

//	gameData.whiteBallTouchInfo.posReleased = { realMouseX, realMouseY };
//	gameData.whiteBallTouchInfo.posTouched = gameData.whiteBall->pos;
//	
//	if (inputData.mouseButtonL == InputData::ButtonState::UP)
//	{
//		gameData.whiteBall->vel = -(gameData.whiteBallTouchInfo.posReleased - gameData.whiteBallTouchInfo.posTouched) * 5.0;
//		gameData.whiteBallTouchInfo.isTouched = false;
//	}
//}

//for (int i = static_cast<int>(Game::RenderData::TextureID::BALL_0);
//	 i < static_cast<int>(Game::RenderData::TextureID::MAX_BALLS);
//	 ++i)
//{
//	RenderData::Sprite sprite {};
//	sprite.position = gameData.balls[i].pos;
//	//sprite.rotation = atan2f (gameData.balls[i].vel.y, gameData.balls[i].vel.x);
//	sprite.size = { gameData.balls[i].scale, gameData.balls[i].scale };
//	sprite.color = { 1.f, 1.f, 1.f };
//	sprite.texture = static_cast<RenderData::TextureID>(i);

//	renderData.sprites.push_back(sprite);
//}

// RENDER DEBUG LINE SHOT
//if (gameData.whiteBallTouchInfo.isTouched)
//{
//	RenderData::Sprite sprite {};
	//auto dir = glm::normalize(gameData.whiteBallTouchInfo.posReleased - gameData.whiteBallTouchInfo.posTouched);
	//auto dist = glm::distance(gameData.whiteBallTouchInfo.posReleased, gameData.whiteBallTouchInfo.posTouched);
	//sprite.position = gameData.whiteBall->pos - dir * dist;
	//sprite.rotation = float(atan2(dir.y, dir.x));
//	//sprite.rotation = glm::radians(sin(gameData.whiteBallTouchInfo.posReleased.x) + cos(gameData.whiteBallTouchInfo.posReleased.y));
//	sprite.size = { dist, 3 };
//	/*char buffer[512];
//	sprintf_s(buffer, "x: %f\n", glm::distance(gameData.whiteBallTouchInfo.posReleased, gameData.whiteBallTouchInfo.posTouched));
//	OutputDebugStringA(buffer);*/
//	sprite.color = { 1.f, 0.f, 0.f };
//	sprite.texture = RenderData::TextureID::PIXEL;

//	renderData.sprites.push_back(sprite);
//}


/*if (posX + GameData::GameObjectScale > inputData.windowHalfSize.x)
				posX = inputData.windowHalfSize.x - GameData::GameObjectScale,
				velX = -velX;
			else if (posX - GameData::GameObjectScale < -inputData.windowHalfSize.x)
				posX = -inputData.windowHalfSize.x + GameData::GameObjectScale,
				velX = -velX;
			if (posY + GameData::GameObjectScale > inputData.windowHalfSize.y)
				posY = inputData.windowHalfSize.y - GameData::GameObjectScale,
				velY = -velY;
			else if (posY - GameData::GameObjectScale < -inputData.windowHalfSize.y)
				posY = -inputData.windowHalfSize.y + GameData::GameObjectScale,
				velY = -velY;*/

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




				//auto jobA = Utilities::TaskManager::CreateLambdaBatchedJob(
				//	[&gameData, &createdGroups](int k, const Utilities::TaskManager::JobContext& contextA)
				//{
				//	gameData.contactGroups[contextA.threadIndex].reserve(gameData.contactDataList[contextA.threadIndex].size());

				//	auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
				//		[&gameData, &createdGroups, &contextA](int i, const Utilities::TaskManager::JobContext& contextB)
				//	{
				//		auto it = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].a); // busquem si ja tenim alguna colisió amb l'objecte "a"
				//		if (it == createdGroups[contextA.threadIndex].end())
				//		{
				//			it = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].b); // busquem si ja tenim alguna colisió amb l'objecte "b"
				//			if (it == createdGroups[contextA.threadIndex].end())
				//			{
				//				gameData.contactGroups[contextA.threadIndex].push_back(GameData::ContactGroup{
				//					{ gameData.contactDataList[contextA.threadIndex][i].a , gameData.contactDataList[contextA.threadIndex][i].b }, // game object indexes
				//					{ gameData.contactDataList[contextA.threadIndex][i] } // ContactData
				//				}); // creem la llista de colisions nova

				//				createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].a] = &gameData.contactGroups[contextA.threadIndex]
				//					[gameData.contactGroups[contextA.threadIndex].size() - 1]; // guardem referència a aquesta llista
				//				createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].b] = &gameData.contactGroups[contextA.threadIndex]
				//					[gameData.contactGroups[contextA.threadIndex].size() - 1]; // per cada objecte per trobarla
				//			}
				//			else
				//			{
				//				it->second->contacts.push_back(gameData.contactDataList[contextA.threadIndex][i]); // afegim la colisió a la llista
				//				it->second->objectIndexes.push_back(gameData.contactDataList[contextA.threadIndex][i].a);
				//				createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].a] = it->second; // guardem referència de l'objecte que no hem trobat abans
				//			}
				//		}
				//		else
				//		{
				//			GameData::ContactGroup *groupA = it->second;
				//			groupA->contacts.push_back(gameData.contactDataList[contextA.threadIndex][i]);

				//			auto itB = createdGroups[contextA.threadIndex].find(gameData.contactDataList[contextA.threadIndex][i].b);
				//			if (itB == createdGroups[contextA.threadIndex].end())
				//			{
				//				groupA->objectIndexes.push_back(gameData.contactDataList[contextA.threadIndex][i].b);
				//				createdGroups[contextA.threadIndex][gameData.contactDataList[contextA.threadIndex][i].b] = groupA;
				//			}
				//			else
				//			{
				//				GameData::ContactGroup *groupB = itB->second;

				//				if (groupA != groupB)
				//				{
				//					// el objecte b ja és a un grup diferent, fem merge dels 2 grups.

				//					// 1 - copiem tot el grup anterior
				//					for (auto& contact : groupB->contacts)
				//					{
				//						groupA->contacts.push_back(contact);
				//					}

				//					// 2 - copiem els elements del segon grup i actualitzem el mapa
				//					//     de grups
				//					for (auto &index : groupB->objectIndexes)
				//					{
				//						if (index != gameData.contactDataList[contextA.threadIndex][i].a)
				//							groupA->objectIndexes.push_back(index);
				//						createdGroups[contextA.threadIndex][index] = groupA;
				//					}

				//					// 3 - marquem el grup com a buit
				//					groupB->objectIndexes.clear();
				//					groupB->contacts.clear();
				//				}
				//			}
				//		}
				//	},
				//	"Generate Contact Groups",
				//	Utilities::Profiler::MaxNumThreads,
				//	Utilities::Profiler::MaxNumThreads);

				//	contextA.DoAndWait(&jobB);
				//},
				//"Generate Contact Groups List",
				//Utilities::Profiler::MaxNumThreads,
				//Utilities::Profiler::MaxNumThreads);
				//context.DoAndWait(&jobA);

//inline void FineGrained(GameData &gameData, const Utilities::TaskManager::JobContext &context)
//{
	//auto size = 0u;
	//for (auto &pcs : gameData.possibleCollisionsSize) size += pcs;
	//gameData.contactDataList.resize(size); // TODO: resize ?

	/*auto jobB = Utilities::TaskManager::CreateLambdaBatchedJob(
	[&gameData](int j, const Utilities::TaskManager::JobContext& context)
	{

	},
	"Generate Contact Data",
	gameData.possibleCollisions[contextA.threadIndex].size() /
	(gameData.possibleCollisions[contextA.threadIndex].size() < Utilities::Profiler::MaxNumThreads ? 1 : Utilities::Profiler::MaxNumThreads),
	gameData.possibleCollisions[contextA.threadIndex].size());

	contextA.DoAndWait(&jobB);*/
//}