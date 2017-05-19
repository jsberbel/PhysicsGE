{
	size_t totalMemory2 = 1 * 1024 * 1024 * 1024; // 1 GB :DDDDDDDDDDD
	
	void* baseAddress2 = reinterpret_cast<void*>(0x30000000000);
	
	uint8_t* gameMemoryBlock2 = reinterpret_cast<uint8_t*>(VirtualAlloc(baseAddress2,
																		totalMemory2,
																		MEM_RESERVE | MEM_COMMIT,
																		PAGE_READWRITE));
	
	Utilities::DefaultAllocator blockAllocator(gameMemoryBlock2, totalMemory2);
	
	
	s_JobScheduler.Init(numThreads, &s_Profiler, &blockAllocator);
}