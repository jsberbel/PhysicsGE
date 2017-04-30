
if(!parallel)
{
	for(int i = 0; groups.size(); ++i)
	{
		solveGroup(groups[i]);
	}
}
else
{
	// paralel
	auto job = CreateLambdaJob(
		[&groups](int i, const JobContext& context)
		{
			solveGroup(groups[i]);
		},
		"grup solver",
		groups.size() // nº de vegades que es farà la tasca.
	);
	
	context.DoAndWait(&job);
}

if(parallel)
{
	// update
	gameData.prevBalls = std::move(gameData.balls);
	gameData.balls.clear();

	// update balls
	for (const auto& ball : gameData.prevBalls)
	{
		glm::dvec2 f = {};
		Ball ballNext = {};
		ballNext.pos = ball.pos + ball.vel * input.dt + f * (0.5 * input.dt * input.dt);
		ballNext.vel = ball.vel + f * input.dt;

		gameData.balls.push_back(ballNext);
	}
}
else
{
	// update
	gameData.prevBalls = std::move(gameData.balls);
	gameData.balls.resize(gameData.balls.size());

	// update balls
	auto job = CreateLambdaJob(
		[&gameData](int i, const JobContext& context)
		{
			const auto& ball = gameData.prevBalls[i];
			
			glm::dvec2 f = {};
			Ball ballNext = {};
			ballNext.pos = ball.pos + ball.vel * input.dt + f * (0.5 * input.dt * input.dt);
			ballNext.vel = ball.vel + f * input.dt;

			gameData.balls[i] = ballNext;
		},
		"update positions job",
		gameData.prevBalls.size());
		
	context.DoAndWait(&job);
}

// sort & sweep
if(parallel)
{
	std::vector<Extreme> list;
	
	for(const GameObject* go : gameObjects)
	{
		list.push_back( { go, dot(go->GetExtreme({-1,0}), {1, 0}), true  } );
		list.push_back( { go, dot(go->GetExtreme({+1,0}), {1, 0}), false } );
	}
}
else
{
	std::vector<Extreme> list;
	list.resize(gameObjects.size() * 2);
	
	auto job = CreateLambdaJob(
		[&gameObjects, &list](int i, const JobContext& context)
		{
			const GameObject* go = gameObjects[i];
			
			list[i * 2 + 0] = { go, dot(go->GetExtreme({-1,0}), {1, 0}), true  };
			list[i * 2 + 1] = { go, dot(go->GetExtreme({+1,0}), {1, 0}), false };
		},
		"Sort & Sweep create list",
		gameObjects.size());
		
	context.DoAndWait(&job);
}


if(parallel)
{
	std::vector<PossibleCollission> result;
	
	for(int i = 0; i < list.size(); ++i)
	{
		if(list[i].min)
		{
			for(int j = i + 1; list[i].go != list[j].go; ++j)
			{
				if(list[j].min)
				{
					result.push_back( { list[i].go, list[j].go } );
				}
			}
		}
	}
	return result;
}
else
{
	std::vector<PossibleCollission> result;
	
	result.resize(gameObjects.size() * gameObjects.size());
	
	std::atomic_int count(0);
	
	//for(int i = 0; i < list.size(); ++i)
	//{
	auto job = CreateLambdaJob(
		[&list, result](int i, const JobContext& context)
		{
			
			if(list[i].min)
			{
				for(int j = i + 1; list[i].go != list[j].go; ++j)
				{
					if(list[j].min)
					{
						int index = count++;
						result[index] = { list[i].go, list[j].go };
					}
				}
			}
		},
		"Sweep",
		list.size());
	
	result.resize(count.load());
	
	return result;
}

{
	std::vector<PossibleCollission> result[maxNumThreads];
	
	auto job = CreateLambdaJob(
		[&list, result](int i, const JobContext& context)
		{
			// TODO la resta
			int index = count[context.threadIndex]++;
			result[context.threadIndex].push_back( { list[i].go, list[j].go } );
		}
		);
		
	// aqui concatenar tots els results
}












