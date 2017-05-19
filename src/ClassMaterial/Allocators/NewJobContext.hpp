struct JobContext
{
	JobScheduler *scheduler;
	Profiler *profiler;
	DefaultAllocator *allocator;
	int threadIndex;
	int fiberIndex;


	void Do(Job* job) const;
	void Wait(Job* job) const;
	void DoAndWait(Job* job) const;

	Profiler::MarkGuard CreateProfileMarkGuard(const char* functionName, int systemID = -1) const
	{
		return profiler->CreateProfileMarkGuard(functionName, threadIndex, systemID);
	}

	virtual void PrintDebug(const char*) const;

	template<typename T, size_t N>
	MemoryBlock<T> alloc(DefaultAllocatorLabel label) const
	{
		return allocator->alloc<T>(label, threadIndex, N);
	}

	MemoryBlock<uint8_t> alloc(DefaultAllocatorLabel label, size_t nbytes) const
	{
		return allocator->alloc<uint8_t>(label, threadIndex, nbytes);
	}

	template<typename T>
	MemoryBlock<T> alloc(DefaultAllocatorLabel label, size_t N = 1) const
	{
		return allocator->alloc<T>(label, threadIndex, N);
	}

	template<typename T>
	MemoryBlock<T> reallocArray(DefaultAllocatorLabel label, const MemoryBlock<T>& oldBlock, size_t N) const
	{
		return allocator->reallocArray<T>(label, threadIndex, oldBlock, N);
	}

	void Free(const DefaultAllocatorLabel& blockLabel) const
	{
		return allocator->Free(blockLabel);
	}

	DefaultAllocatorLabel GetUnusedLabel(const char* systemName) const
	{
		return allocator->GetUnusedLabel(systemName);
	}

	void ReturnLabel(DefaultAllocatorLabel label) const
	{
		allocator->ReturnLabel(label);
	}

	DefaultAllocator::LabelGuard GetUnusedLabelWithGuard(const char* systemName) const
	{
		return allocator->GetUnusedLabelWithGuard(systemName);
	}

	JobAllocator CreateJobAllocator(const char* labelName) const
	{
		return JobAllocator(allocator, threadIndex, labelName);
	}
};