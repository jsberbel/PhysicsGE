#include <Windows.h>
#include "TaskManager.hh"

{
	// update

	bool hasFinishedUpdating = false;

	auto updateJob = CreateLambdaJob([&](int, const JobContext &context)
	{
		for (int i = 0; i < numFramesElapsed; ++i)
		{
			l_Input.dt = (float)l_TicksPerFrame / (float)l_PerfCountFrequency;

			Update(l_Input, *l_GameData, context);

			l_Input.clickDown = false;
			l_Input.clickDownRight = false;

			LARGE_INTEGER l_UpdateTime;
			QueryPerformanceCounter(&l_UpdateTime);
			int64_t updateTicks = l_UpdateTime.QuadPart - l_CurrentTime.QuadPart;
			double ticksPerUpdate = (double)updateTicks / (i + 1);
			if (ticksPerUpdate * (i + 2) > l_TicksPerFrame)
			{
				break;
			}
		}
		{
			std::unique_lock<std::mutex> lock(frameLockMutex);
			hasFinishedUpdating = true;
		}
		frameLockConditionVariable.notify_all();
	}, "Update", 1, 0, Job::Priority::HIGH, true);

	s_JobScheduler.Do(&updateJob, nullptr);

	{
		std::unique_lock<std::mutex> lock(frameLockMutex);

		while (!hasFinishedUpdating)
		{
			frameLockConditionVariable.wait(lock);
		}
	}

	Render();
}