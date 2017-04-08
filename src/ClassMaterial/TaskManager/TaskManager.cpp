#include <Windows.h>

class WinJobScheduler : public JobScheduler
{
public:
	

	void SwitchToFiber(void* fiber) override
	{
		::SwitchToFiber(fiber);
	}

	void* CreateFiber(size_t stackSize, void(__stdcall*call) (void*), void* parameter) override
	{
		return ::CreateFiber(stackSize, call, parameter);
	}

	void* GetFiberData() const override
	{
		return ::GetFiberData();
	}
};

WinJobScheduler s_JobScheduler;
Profiler s_Profiler;



inline void SetThreadName(unsigned long dwThreadID, const char* threadName)
{
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
	struct
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	} info;
#pragma pack(pop)  

	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
	}
#pragma warning(pop)  
}

void __stdcall WorkerThread(int idx)
{
	{
		char buffer[128] = { "Worker Thread " };
		_itoa(idx, &buffer[strlen(buffer)], 10);
		SetThreadName(GetCurrentThreadId(), buffer);

		//auto hr = SetThreadAffinityMask(GetCurrentThread(), 1LL << idx);
		//assert(hr != 0);
	}

	s_JobScheduler.SetRootFiber(ConvertThreadToFiber(nullptr), idx);


	s_JobScheduler.RunScheduler(idx, s_Profiler);
}

{
	// init worker threads
	unsigned int numHardwareCores = std::thread::hardware_concurrency();
	numHardwareCores = numHardwareCores > 1 ? numHardwareCores - 1 : 1;
	int numThreads = (numHardwareCores < MaxNumThreads-1) ? numHardwareCores : MaxNumThreads-1;
	std::thread *workerThread = (std::thread*)alloca(sizeof(std::thread) * numThreads);

	JobContext mainThreadContext;// {nullptr, &blockAllocator, numThreads, -1};
	mainThreadContext.scheduler = nullptr;
	mainThreadContext.profiler = &s_Profiler;
	mainThreadContext.allocator = &blockAllocator;
	mainThreadContext.threadIndex = numThreads;
	mainThreadContext.fiberIndex = -1;

	s_JobScheduler.Init(numThreads, &s_Profiler, &blockAllocator);
	
	for (int i = 0; i < numThreads; ++i)
	{
		new(&workerThread[i]) std::thread(WorkerThread, i);
	}

	std::mutex frameLockMutex;
	std::condition_variable frameLockConditionVariable;
}


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
	}, "Update", 1, 0, Job::Priority::HIGH, true );

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