#pragma once
#include "TaskManager.hh"
#include "Allocators.hpp"

struct JobContext
{
	Utilities::TaskManager::JobScheduler *scheduler;
	Utilities::Profiler *profiler;
	Utilities::DefaultAllocator *allocator;
	int threadIndex;
	int fiberIndex;

	void Do(Utilities::TaskManager::Job* job) const;
	void Wait(Utilities::TaskManager::Job* job) const;
	void DoAndWait(Utilities::TaskManager::Job* job) const;

	Utilities::Profiler::MarkGuard CreateProfileMarkGuard(const char* functionName, int systemID = -1) const
	{
		return profiler->CreateProfileMarkGuard(functionName, threadIndex, systemID);
	}

	virtual void PrintDebug(const char*) const;

	template<typename T, size_t N>
	Utilities::MemoryBlock<T> alloc(Utilities::DefaultAllocatorLabel label) const
	{
		return allocator->alloc<T>(label, threadIndex, N);
	}

	Utilities::MemoryBlock<uint8_t> alloc(Utilities::DefaultAllocatorLabel label, size_t nbytes) const
	{
		return allocator->alloc<uint8_t>(label, threadIndex, nbytes);
	}

	template<typename T>
	Utilities::MemoryBlock<T> alloc(Utilities::DefaultAllocatorLabel label, size_t N = 1) const
	{
		return allocator->alloc<T>(label, threadIndex, N);
	}

	template<typename T>
	Utilities::MemoryBlock<T> reallocArray(Utilities::DefaultAllocatorLabel label, const Utilities::MemoryBlock<T>& oldBlock, size_t N) const
	{
		return allocator->reallocArray<T>(label, threadIndex, oldBlock, N);
	}

	void Free(const Utilities::DefaultAllocatorLabel& blockLabel) const
	{
		return allocator->Free(blockLabel);
	}

	Utilities::DefaultAllocatorLabel GetUnusedLabel(const char* systemName) const
	{
		return allocator->GetUnusedLabel(systemName);
	}

	void ReturnLabel(Utilities::DefaultAllocatorLabel label) const
	{
		allocator->ReturnLabel(label);
	}

	Utilities::DefaultAllocator::LabelGuard GetUnusedLabelWithGuard(const char* systemName) const
	{
		return allocator->GetUnusedLabelWithGuard(systemName);
	}

	/*JobAllocator CreateJobAllocator(const char* labelName) const
	{
		return JobAllocator(allocator, threadIndex, labelName);
	}*/
};
