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