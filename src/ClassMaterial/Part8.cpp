

	LARGE_INTEGER l_LastFrameTime;
	int64_t l_PerfCountFrequency;
	{
		LARGE_INTEGER PerfCountFrequencyResult;
		QueryPerformanceFrequency(&PerfCountFrequencyResult);
		l_PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
	}

	int64_t l_TicksPerFrame = l_PerfCountFrequency / 30;

	{
		// init time
		QueryPerformanceCounter(&l_LastFrameTime);
		
	}
	{
		LARGE_INTEGER l_CurrentTime;
		QueryPerformanceCounter(&l_CurrentTime);
		//float Result = ((float)(l_CurrentTime.QuadPart - l_LastFrameTime.QuadPart) / (float)l_PerfCountFrequency);

		if (l_LastFrameTime.QuadPart + l_TicksPerFrame > l_CurrentTime.QuadPart)
		{
			int64_t ticksToSleep = l_LastFrameTime.QuadPart + l_TicksPerFrame - l_CurrentTime.QuadPart;
			int64_t msToSleep = 1000 * ticksToSleep / l_PerfCountFrequency;
			if (msToSleep > 0)
			{
				Sleep((DWORD)msToSleep);
			}
			continue;
		}
		while (l_LastFrameTime.QuadPart + l_TicksPerFrame <= l_CurrentTime.QuadPart)
		{
			l_LastFrameTime.QuadPart += l_TicksPerFrame;
		}

		double dt = (double)l_TicksPerFrame / (double)l_PerfCountFrequency;
		
		

		position.x += horizontal * dt;
		position.y += vertical * dt;
	}