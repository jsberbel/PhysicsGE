#pragma once

#include <new>
#include <atomic>
#include <mutex>
#include "SOA.hpp"

namespace Utilities {
	template<size_t size, size_t align = 16, size_t buckets = 256>
	struct FixedAllocator
	{
		struct alignas(align)Data {
			uint8_t d[size];
		} data[buckets];
		uint8_t freeBuckets[buckets];
		uint16_t next = 0;

		static_assert(sizeof(Data) == size, "Data must have a size of size... :-/");
		static_assert(buckets <= 256, "can only support 256 buckets");

		FixedAllocator()
		{
			for (int i = 0; i < buckets; ++i)
			{
				freeBuckets[i] = i;
			}
		}

		void* alloc()
		{
			if (next < buckets)
			{
				int index = freeBuckets[next];
				++next;
				uint8_t* result = data[index].d;
				assert(result >= data[0].d);
				assert(result <= data[buckets].d);
				return result;
			}
			else
			{
				return nullptr;
			}
		}

		bool free(void* ptr)
		{
			intptr_t index = (reinterpret_cast<intptr_t>(ptr) - reinterpret_cast<intptr_t>(data)) / sizeof(Data::d);
			if (index >= 0 && index < buckets)
			{
				assert(ptr == data[index].d);
				assert(next > 0);
				--next;
				freeBuckets[next] = (uint8_t)index;
				return true;
			}
			assert(false);
			return false;
		}
	};

	template<size_t poolSize, size_t align = 16, size_t partitionSize = 256>
	struct MemoryPool
	{
		static_assert(partitionSize % align == 0, "partitions must be divisible by align");

		alignas(align) uint8_t buffer[poolSize];

		struct Tail;

		struct Node
		{
			size_t size;
			bool isFreeNode;

			Tail* getTail()
			{
				uint8_t *ptr = (uint8_t*)this;
				ptr += size - sizeof(Tail);
				return (Tail*)ptr;
			}
		};

		struct FreeNode : Node
		{
			FreeNode *next, *prev;
		};

		struct Tail
		{
			bool isFreeNode;
			union {
				FreeNode *freeHead;
				Node *head;
			};
		};

		FreeNode root;
		size_t usedMemory = 0;

		MemoryPool()
		{
			FreeNode *firstNode = (FreeNode*)buffer;

			firstNode->next = firstNode->prev = &root;
			root.next = root.prev = firstNode;

			firstNode->size = poolSize;
			root.size = 0;

			Tail* firstTail = firstNode->getTail();
			firstTail->freeHead = firstNode;

			root.isFreeNode = firstNode->isFreeNode = firstTail->isFreeNode = true;
		}

		constexpr static size_t ResizeToMultiple(size_t size, size_t multiple)
		{
			return (((size - 1) / multiple) + 1) * multiple;
		}

		void* alloc(size_t size)
		{
			size_t minSize = size + ResizeToMultiple(sizeof(Node), align) + sizeof(Tail);
			minSize = ResizeToMultiple(minSize, partitionSize);

			FreeNode *node = root.next;
			while (node->size < minSize)
			{
				node = node->next;
				if (node == &root)
					return nullptr; // not enought space
			}

			FreeNode *prev = node->prev;
			FreeNode *next = node->next;


			if (node->size != minSize)
			{
				FreeNode *newNode = (FreeNode*)(((uint8_t*)node + minSize));
				newNode->isFreeNode = true;
				newNode->size = node->size - minSize;
				newNode->prev = prev;
				newNode->next = next;

				prev->next = newNode;
				next->prev = newNode;

				Tail* newNodeTail = newNode->getTail();
				assert(newNodeTail == node->getTail());
				assert(newNodeTail->isFreeNode);
				assert(newNodeTail->freeHead == node);
				newNodeTail->isFreeNode = true;
				newNodeTail->freeHead = newNode;
			}
			else
			{
				prev->next = next;
				next->prev = prev;
			}

			node->size = minSize;

			Tail* tail = node->getTail();
			tail->isFreeNode = node->isFreeNode = false;
			tail->head = node;

			uint8_t* result = (uint8_t*)node + ResizeToMultiple(sizeof(Node), align);

			assert(result > buffer);
			assert(result + size < buffer + poolSize);

			usedMemory += minSize;
			return result;
		}

		void free(void* ptr)
		{
			FreeNode* node = (FreeNode*)((uint8_t*)ptr - ResizeToMultiple(sizeof(Node), align));
			assert(!node->isFreeNode);
			FreeNode* prev = GetAdjacentPreviousFreeNode(node);
			FreeNode* next = GetAdjacentNextFreeNode(node);

			usedMemory -= node->size;

			if (prev != nullptr && next != nullptr)
			{
				// merge both sides
				prev->size += node->size + next->size;
				node = prev;
				node->getTail()->freeHead = node;

				prev = prev->prev;
				next = next->next;
			}
			else if (prev != nullptr)
			{
				// merge with prev
				prev->size += node->size;
				node = prev;
				node->getTail()->freeHead = node;

				next = prev->next;
				prev = prev->prev;
			}
			else if (next != nullptr)
			{
				// merge with next
				node->size += next->size;
				node->getTail()->freeHead = node;

				prev = next->prev;
				next = next->next;
			}
			else
			{
				// only one search is necessary, but which one to choose?
				prev = GetPreviousFreeNode(node);
				// next = GetNextFreeNode(node);
				next = prev->next;
			}

			prev->next = node;
			next->prev = node;

			node->next = next;
			node->prev = prev;

			node->isFreeNode = true;
			node->getTail()->isFreeNode = true;

		}

	private:
		FreeNode* GetAdjacentPreviousFreeNode(Node* node)
		{
			if ((uint8_t*)node == buffer/* || (uint8_t*)node + node->size == buffer + poolSize*/)
			{
				return nullptr;
			}

			Tail* prevTail = (Tail*)(((uint8_t*)node - sizeof(Tail)));
			return (prevTail->isFreeNode) ? prevTail->freeHead : nullptr;
		}
		FreeNode* GetAdjacentNextFreeNode(Node* node)
		{
			if ((uint8_t*)node + node->size == buffer + poolSize)
			{
				return nullptr;
			}

			Node* next = (Node*)(((uint8_t*)node + node->size));
			return (next->isFreeNode) ? (FreeNode*)next : nullptr;
		}


		FreeNode* GetPreviousFreeNode(Node* node)
		{
			if ((uint8_t*)node == buffer)
			{
				return &root;
			}

			Tail* prevTail = (Tail*)(((uint8_t*)node - sizeof(Tail)));
			return (prevTail->isFreeNode) ? prevTail->freeHead : GetPreviousFreeNode(prevTail->head);
		}
		FreeNode* GetNextFreeNode(Node* node)
		{
			if ((uint8_t*)node + node->size == buffer + poolSize)
			{
				return &root;
			}

			Node* next = (Node*)(((uint8_t*)node + node->size));
			return (next->isFreeNode) ? (FreeNode*)next : GetNextFreeNode(next);
		}
	};

	struct PermanentMemoryPool
	{
		size_t size, currentOffset = 0;
		uint8_t *buffer;

		template<typename T>
		T* alloc()
		{
			uintptr_t ptr = reinterpret_cast<uintptr_t>(buffer) + currentOffset;

			uintptr_t finalptr = (ptr + alignof(T)- 1) & ~(alignof(T)- 1);

			if (finalptr + sizeof(T) > reinterpret_cast<uintptr_t>(buffer) + size)
			{
				assert(false); // Not enough memory!
				return nullptr;
			}
			else
			{
				currentOffset = finalptr + sizeof(T) - reinterpret_cast<uintptr_t>(buffer);
				return reinterpret_cast<T*>(finalptr);
			}
		}

		uint8_t* alloc(size_t nbytes)
		{
			uintptr_t ptr = reinterpret_cast<uintptr_t>(buffer) + currentOffset;
			if (ptr + nbytes > reinterpret_cast<uintptr_t>(buffer) + size)
			{
				assert(false); // Not enough memory!
				return nullptr;
			}
			else
			{
				currentOffset += nbytes;
				return reinterpret_cast<uint8_t*>(ptr);
			}
		}
	};

	template<typename BlockLabelType, size_t BlockSize>
	struct LabeledBlockAllocator
	{
		LabeledBlockAllocator() = delete;
		LabeledBlockAllocator(const LabeledBlockAllocator& original) = delete;
		LabeledBlockAllocator& operator=(const LabeledBlockAllocator& original) = delete;

		LabeledBlockAllocator(uint8_t *buffer, size_t size, bool isBufferAlignedToBlockSize = true)
			: memoryBlocks(reinterpret_cast<Block*>(buffer))
			, firstBlock(nullptr)
		{
			assert(!isBufferAlignedToBlockSize || (reinterpret_cast<uintptr_t>(buffer) == ((reinterpret_cast<uintptr_t>(buffer) + BlockSize - 1) & ~(BlockSize - 1))));

			int numBlocks = (int)(size / BlockSize);

			MemoryControlBlock *lastFreeBlock = reinterpret_cast<MemoryControlBlock*>((reinterpret_cast<uintptr_t>(buffer) + size + alignof(MemoryControlBlock)-1) & ~(alignof(MemoryControlBlock)-1));
			if (reinterpret_cast<uint8_t*>(lastFreeBlock) > buffer + size)
				--lastFreeBlock;

			firstFreeBlock = lastFreeBlock - numBlocks;
			while (&buffer[numBlocks * BlockSize] > reinterpret_cast<uint8_t*>(firstFreeBlock))
			{
				--numBlocks;
				firstFreeBlock = lastFreeBlock - numBlocks;
			}
			assert(numBlocks > 0);
			freeBlocks = numBlocks;

			for (int i = 0; i < numBlocks; ++i)
			{
				firstFreeBlock[i].index = i;
				firstFreeBlock[i].next = &firstFreeBlock[i + 1];
			}
			firstFreeBlock[numBlocks - 1].next = nullptr;
		}

		uint8_t* GetBlock(const BlockLabelType& blockLabel)
		{
			if (firstFreeBlock == nullptr)
			{
				return nullptr;
			}
			--freeBlocks;

			MemoryControlBlock *controlBlock = firstFreeBlock;
			firstFreeBlock = controlBlock->next;

			controlBlock->next = firstBlock;
			firstBlock = controlBlock;
			controlBlock->name = blockLabel;
			return reinterpret_cast<uint8_t*>(&memoryBlocks[controlBlock->index]);
		}

		uint8_t* GetConsecutiveBlocks(const BlockLabelType& blockLabel, int n)
		{
			if (firstFreeBlock == nullptr)
			{
				return nullptr;
			}

			// memory block will go from "firstCandidate" to "lastCandidate"
			MemoryControlBlock *firstCandidate = firstFreeBlock, *lastCandidate = firstFreeBlock, *firstFailedCandidate = nullptr, *lastFailedCandidate = nullptr;

			firstFreeBlock = firstCandidate->next;
			firstCandidate->next = nullptr;
			firstCandidate->name = blockLabel;

			size_t firstBlockIndex = firstCandidate->index, lastBlockIndex = firstCandidate->index;
			do
			{
				MemoryControlBlock *lastCandidateFound = FindNeightbourBlock(firstBlockIndex, lastBlockIndex);
				if (lastCandidateFound == nullptr)
				{
					assert(firstFreeBlock != nullptr);
					// RESET
					lastCandidate->next = firstFailedCandidate;
					firstFailedCandidate = firstCandidate;
					if(lastFailedCandidate == nullptr)
						lastFailedCandidate = lastCandidate;

					firstCandidate = firstFreeBlock;
					lastCandidate = firstFreeBlock;

					firstFreeBlock = firstCandidate->next;
					firstCandidate->next = nullptr;
					firstCandidate->name = blockLabel;

					firstBlockIndex = firstCandidate->index;
					lastBlockIndex = firstCandidate->index;
					continue;
				}
				else if (lastCandidateFound->index == firstBlockIndex - 1)
				{
					firstBlockIndex = lastCandidateFound->index;

					lastCandidateFound->next = firstCandidate;
					firstCandidate = lastCandidateFound;
				}
				else
				{
					assert(lastCandidateFound->index == lastBlockIndex + 1);
					assert(lastCandidate->next == nullptr);

					lastBlockIndex = lastCandidateFound->index;

					lastCandidate->next = lastCandidateFound;
					lastCandidate = lastCandidateFound;
				}
				lastCandidateFound->name = blockLabel;
			} while ((lastBlockIndex + 1) - firstBlockIndex < n);

			if (firstFailedCandidate != nullptr)
			{
				assert(lastFailedCandidate->next == nullptr);
				lastFailedCandidate->next = firstFreeBlock;
				firstFreeBlock = firstFailedCandidate;
			}
			else
			{
				assert(lastFailedCandidate == nullptr);
			}

			lastCandidate->next = firstBlock;
			firstBlock = firstCandidate;

			freeBlocks -= n;
			return reinterpret_cast<uint8_t*>(&memoryBlocks[firstCandidate->index]);
		}

		void Free(const BlockLabelType& blockLabel)
		{
			MemoryControlBlock **ptrToNext = &firstBlock;
			while (*ptrToNext != nullptr)
			{
				if ((*ptrToNext)->name == blockLabel) // remove next if it has the name we are searching for
				{
					MemoryControlBlock *controlBlock = *ptrToNext;
					*ptrToNext = controlBlock->next;

					controlBlock->next = firstFreeBlock;
					firstFreeBlock = controlBlock;

					++freeBlocks;
				}
				else
				{
					ptrToNext = &(*ptrToNext)->next; // move to the next
				}
			}
		}

	private:

		struct Block {
			uint8_t block[BlockSize];
		} *memoryBlocks;

		struct MemoryControlBlock
		{
			BlockLabelType name;
			size_t index;
			MemoryControlBlock* next;
		};

		MemoryControlBlock *firstFreeBlock;
		MemoryControlBlock *firstBlock;
		int freeBlocks;


		MemoryControlBlock* FindNeightbourBlock(size_t firstIndex, size_t lastIndex)
		{
			if (firstFreeBlock->index == firstIndex - 1 || firstFreeBlock->index == lastIndex + 1)
			{
				MemoryControlBlock* block = firstFreeBlock;
				firstFreeBlock = block->next;
				block->next = nullptr;
				return block;
			}

			for (MemoryControlBlock* block = firstFreeBlock->next, *prevBlock = firstFreeBlock; block != nullptr; prevBlock = block, block = block->next)
			{
				if (block->index == firstIndex - 1 || block->index == lastIndex + 1)
				{
					prevBlock->next = block->next;
					block->next = nullptr;
					return block;
				}
			}
			return nullptr;
		}
	};


	template<typename T>
	struct MemoryBlock
	{
		T* ptr;
		size_t size;
		operator T*() { return ptr; }
		T& operator[](size_t i) { return ptr[i]; }
		const T& operator[](size_t i) const { return ptr[i]; }
		bool operator==(nullptr_t) { return ptr == nullptr; }
		bool operator!=(nullptr_t) { return ptr != nullptr; }
	};

	struct StackAllocator
	{
		const size_t size;
		size_t currentOffset = 0, lastOffset = 0;
		uint8_t * const buffer;
		uint8_t * lastAlloc;

		StackAllocator() = delete;
		StackAllocator(uint8_t *buffer, size_t size) : buffer(buffer), size(size) {}
		StackAllocator(const StackAllocator& original) = default;
		StackAllocator& operator=(const StackAllocator& original) = default;

		template<typename T>
		MemoryBlock<T> alloc(size_t N = 1)
		{
			uintptr_t ptr = reinterpret_cast<uintptr_t>(buffer) + currentOffset;

			uintptr_t finalptr = (ptr + alignof(T)-1) & ~(alignof(T)-1);
			uintptr_t ptr2 = reinterpret_cast<uintptr_t>(&reinterpret_cast<T*>(finalptr)[N]);

			if (ptr2 > reinterpret_cast<uintptr_t>(buffer) + size)
			{
				return MemoryBlock<T>{nullptr, 0};
			}
			else
			{
				currentOffset = ptr2 - reinterpret_cast<uintptr_t>(buffer);
				lastAlloc = reinterpret_cast<uint8_t*>(finalptr);
				return MemoryBlock<T>{ reinterpret_cast<T*>(finalptr), sizeof(T) * N };
			}
		}

		template<typename T>
		MemoryBlock<T> reallocArray(MemoryBlock<T> oldBlock, size_t N)
		{
			if (oldBlock.ptr == reinterpret_cast<T*>(lastAlloc))
			{
				if (reinterpret_cast<uintptr_t>(lastAlloc) + sizeof(T) * N > reinterpret_cast<uintptr_t>(buffer) + size)
				{
					return MemoryBlock<T>{nullptr, 0};
				}
				else
				{
					if (sizeof(T) * N > oldBlock.size)
						currentOffset += sizeof(T) * N - oldBlock.size;
					else
						currentOffset -= oldBlock.size - sizeof(T) * N;
					return MemoryBlock<T>{ reinterpret_cast<T*>(lastAlloc), sizeof(T) * N };
				}
			}
			else
			{
				MemoryBlock<T> result = alloc<T>(N);
				if (result.ptr != nullptr)
				{
					for (int i = 0; i < N && i * sizeof(T) < oldBlock.size; ++i)
					{
						result.ptr[i] = std::move(oldBlock.ptr[i]);
					}
				}
				return result;
			}
		}


		void Push()
		{
			size_t savedOffset = currentOffset;
			size_t *offset = alloc<size_t>();
			*offset = lastOffset;
			lastOffset = savedOffset;
			lastAlloc = nullptr;
		}

		void Pop()
		{
			size_t savedOffset = currentOffset = lastOffset;
			size_t *offset = alloc<size_t>();
			lastOffset = *offset;
			currentOffset = savedOffset;
			lastAlloc = nullptr;
		}

		struct Guard
		{
			StackAllocator *allocator;
			Guard() = delete;
			Guard(StackAllocator &allocator) : allocator(&allocator) { allocator.Push(); }
			Guard(const Guard&) = delete;
			Guard& operator=(const Guard&) = delete;
			Guard(Guard&& other) : allocator(other.allocator) { other.allocator = nullptr; }
			~Guard() { if (allocator != nullptr) { allocator->Pop(); } }
		};

		Guard PushWithGuard()
		{
			return Guard(*this);
		}
	};

	template<size_t N>
	struct OnStackAllocator : StackAllocator
	{
		uint8_t buffer[N];
		OnStackAllocator() : StackAllocator(buffer, N) {}
	};


	template<typename BlockLabelType, size_t BlockSize, size_t MaxNumThreads>
	struct ThreadedLabeledBlockAllocator
	{
		static constexpr BlockLabelType InternalDataLabel = {};

		ThreadedLabeledBlockAllocator() = delete;
		ThreadedLabeledBlockAllocator(const ThreadedLabeledBlockAllocator& original) = delete;
		ThreadedLabeledBlockAllocator& operator=(const ThreadedLabeledBlockAllocator& original) = delete;

		ThreadedLabeledBlockAllocator(uint8_t *buffer, size_t size)
			: base(buffer, size)
			, internalMemoryAllocator(base.GetBlock(InternalDataLabel), BlockSize)
		{

		}

		template<typename T>
		MemoryBlock<T> alloc(BlockLabelType label, int threadId, size_t N = 1)
		{
			if (sizeof(T) * N < BlockSize)
			{
				return InnerAlloc<T>(label, threadId, [N](StackAllocator* allocator) { return allocator->alloc<T>(N); });
			}
			else
			{
				std::unique_lock<std::mutex> lock(mutex);
				return LargeAlloc<T>(label, N);
			}
		}

		template<typename T>
		MemoryBlock<T> reallocArray(BlockLabelType label, int threadId, const MemoryBlock<T>& oldBlock, size_t N)
		{
			if (sizeof(T) * N < BlockSize)
			{
				return InnerAlloc<T>(label, threadId, [&oldBlock, N](StackAllocator* allocator) { return allocator->reallocArray<T>(oldBlock, N); });
			}
			else
			{
				size_t oldSize = sizeof(T) * N;
				int oldNumBlocks = (int)((oldSize - 1) / BlockSize) + 1;

				size_t requestedSize = sizeof(T) * N;
				int numBlocks = (int)((requestedSize - 1) / BlockSize) + 1;

				if (oldNumBlocks > 1 && oldNumBlocks == numBlocks)
				{
					return { oldBlock.ptr, sizeof(T) * N };
				}
				else
				{
					// TODO can be MUCH smarter
					std::unique_lock<std::mutex> lock(mutex);
					MemoryBlock<T> result = LargeAlloc<T>(label, N);

					for (int i = 0; i < N && i * sizeof(T) < oldBlock.size; ++i)
					{
						result.ptr[i] = std::move(oldBlock.ptr[i]);
					}

					return result;
				}
			}
		}

		void Free(const BlockLabelType& blockLabel)
		{
			assert(blockLabel != InternalDataLabel);
			std::unique_lock<std::mutex> lock(mutex);
			base.Free(blockLabel);

			for (int t = 0; t < MaxNumThreads; ++t)
			{
				PerThreadInfo* threadInfo = perThreadInfo[t];
				while (threadInfo != nullptr)
				{
					auto allocator = threadInfo->memoryAllocators.Get(blockLabel);
					if (allocator != nullptr)
					{
						// Trust me, I'm an engineer (-:
						StackAllocator* newAllocator = new(*allocator.data)StackAllocator(nullptr, 0);
						assert(newAllocator == *allocator.data);
						break;
					}

					threadInfo = threadInfo->next;
				}
			}

		}

		BlockLabelType GetUnusedLabel(const char* systemName)
		{
			FreeLabel* freeLabel;
			// TODO this can be lockless most of the time
			if (firstFreeLabel == nullptr)
			{
				freeLabel = InternalAlloc<FreeLabel>();

				std::unique_lock<std::mutex> lock(mutex);
				freeLabel->label = ++lastUsedLabel;
				assert(lastUsedLabel != InternalDataLabel);

				freeLabel->nextLabel = firstUsedLabel;
				firstUsedLabel = freeLabel;
			}
			else
			{
				std::unique_lock<std::mutex> lock(mutex);

				freeLabel = firstFreeLabel;
				firstFreeLabel = freeLabel->nextLabel;

				freeLabel->nextLabel = firstUsedLabel;
				firstUsedLabel = freeLabel;
			}

			freeLabel->systemName = systemName;
			return freeLabel->label;
		}

		void ReturnLabel(BlockLabelType label)
		{
			assert(firstUsedLabel != nullptr);

			// TODO this can be lockless all the time
			std::unique_lock<std::mutex> lock(mutex);
			FreeLabel* freeLabel = firstUsedLabel;
			firstUsedLabel = freeLabel->nextLabel;


			freeLabel->label = label;
			freeLabel->nextLabel = firstFreeLabel;
			firstFreeLabel = freeLabel;
		}

		struct LabelGuard
		{
			BlockLabelType label;

			LabelGuard(BlockLabelType _label, ThreadedLabeledBlockAllocator *_allocator) : label(_label), allocator(_allocator) {}
			LabelGuard(const LabelGuard&) = delete;
			LabelGuard& operator=(const LabelGuard&) = delete;
			LabelGuard(LabelGuard&& other) : label(other.label), allocator(other.allocator) { other.label = InternalDataLabel; other.allocator = nullptr; }
			LabelGuard& operator=(LabelGuard&& other) = delete;
			~LabelGuard() { if (allocator != nullptr) { allocator->Free(label); allocator->ReturnLabel(label); } }

			operator BlockLabelType() const { return label; }
		private:
			ThreadedLabeledBlockAllocator *allocator;
		};

		LabelGuard GetUnusedLabelWithGuard(const char* systemName)
		{
			LabelGuard result(GetUnusedLabel(systemName), this);
			return result;
		}

	private:


		template<typename T, typename AllocFunc>
		MemoryBlock<T> InnerAlloc(BlockLabelType label, int threadId, const AllocFunc& allocFunc, MemoryBlock<T> oldBlock = {})
		{
			assert(label != InternalDataLabel);
			assert(threadId >= 0);
			assert(threadId < MaxNumThreads);
			StackAllocator* allocator = GetStackAllocator(label, threadId);

			MemoryBlock<T> p = allocFunc(allocator);
			if (p.ptr == nullptr)
			{
				assert(oldBlock.ptr == nullptr); // TODO realloc here

				RefillAllocator(allocator, label);
				MemoryBlock<T> p2 = allocFunc(allocator);
				assert(p2.ptr != nullptr);
				return p2;
			}
			else
			{
				return p;
			}
		}

		template<typename T>
		MemoryBlock<T> LargeAlloc(BlockLabelType label, size_t num = 1)
		{
			assert(BlockSize < sizeof(T) * num);
			size_t requestedSize = sizeof(T) * num;
			int numBlocks = (int)((requestedSize - 1) / BlockSize) + 1;
			assert(numBlocks > 1);
			assert(BlockSize * numBlocks >= sizeof(T) * num);
			assert(BlockSize * (numBlocks - 1) < sizeof(T) * num);
			uint8_t *ptr = base.GetConsecutiveBlocks(label, numBlocks);
			return MemoryBlock<T>{ reinterpret_cast<T*>(ptr), BlockSize * numBlocks };
		}

		StackAllocator* GetStackAllocator(BlockLabelType label, int threadId)
		{
			assert(label != InternalDataLabel);
			assert(threadId >= 0);
			assert(threadId < MaxNumThreads);

			PerThreadInfo* threadInfo = perThreadInfo[threadId], *lastThreadInfo = nullptr;
			for(;;)
			{
				if (threadInfo == nullptr)
				{
					threadInfo = InternalAlloc<PerThreadInfo>();
					PerThreadInfo* newThreadInfo = new(threadInfo)PerThreadInfo();
					assert(newThreadInfo == threadInfo);
					if (lastThreadInfo == nullptr)
						perThreadInfo[threadId] = threadInfo;
					else
						lastThreadInfo->next = threadInfo;
				}

				auto allocator = threadInfo->memoryAllocators.Get(label);
				if (allocator != nullptr)
				{
					return *allocator.data;
				}
				else if (threadInfo->memoryAllocators.Occupancy() < 0.66f)
				{
					StackAllocator *allocator = InternalAlloc<StackAllocator>();
					*threadInfo->memoryAllocators.Reserve(label).data = allocator;
					RefillAllocator(allocator, label);
					return allocator;
				}
				else if (threadInfo->next == nullptr)
				{
					threadInfo->next = InternalAlloc<PerThreadInfo>();
				}
				
				lastThreadInfo = threadInfo;
				threadInfo = threadInfo->next;
			}
		}

		void RefillAllocator(StackAllocator* allocator, BlockLabelType label)
		{
			assert(label != InternalDataLabel);
			std::unique_lock<std::mutex> lock(mutex);

			// Trust me, I'm an engineer (-:
			StackAllocator* newAllocator = new(allocator)StackAllocator(base.GetBlock(label), BlockSize);
			assert(newAllocator == allocator);
		}

		template<typename T>
		T* InternalAlloc()
		{
			std::unique_lock<std::mutex> lock(mutex);
			
			T* p = internalMemoryAllocator.alloc<T>();
			while (p == nullptr)
			{
				// Trust me, I'm an engineer (-:
				StackAllocator* newAllocator = new(&internalMemoryAllocator)StackAllocator(base.GetBlock(InternalDataLabel), BlockSize);
				assert(newAllocator == &internalMemoryAllocator);

				p = internalMemoryAllocator.alloc<T>();
			}

			return p;
		}

		struct PassThroughHasher
		{
			uint64_t operator()(const BlockLabelType& t) { return ((uint64_t)t) * 17; } // a prime number to mix it up
		};

		std::mutex mutex;
		LabeledBlockAllocator<BlockLabelType, BlockSize> base;
		StackAllocator internalMemoryAllocator;
		struct PerThreadInfo
		{
			HashMap<BlockLabelType, StackAllocator*, 63, PassThroughHasher> memoryAllocators;
			PerThreadInfo* next;
		} *perThreadInfo[MaxNumThreads] = {};

		struct FreeLabel
		{
			BlockLabelType label;
			const char* systemName;
			FreeLabel *nextLabel;
		};

		FreeLabel* firstFreeLabel = nullptr, *firstUsedLabel = nullptr;
		BlockLabelType lastUsedLabel = InternalDataLabel;



		// Ideas:
		//  we can add a linked list of allocators in "perThreadInfo->memoryAllocators"
		//  so before creating a new allocator we try the previous ones for one with enought space. Probably wasterfull.
	};



	template<typename Allocator>
	struct AllocatorWrapper
	{
		Allocator allocator;

		template<typename T>
		MemoryBlock<T> alloc(int N)
		{
			return allocator.alloc<T>(N);
		}

		template<typename T, size_t N = 1>
		MemoryBlock<T> alloc()
		{
			return allocator.alloc<T>(N);
		}

		template<typename T>
		MemoryBlock<T> realloc(MemoryBlock<T> oldBlock, size_t N)
		{
			return allocator.realloc<T>(oldBlock, N);
		}

		template<typename T, size_t N>
		MemoryBlock<T> realloc(MemoryBlock<T> oldBlock)
		{
			return allocator.realloc<T>(oldBlock, N);
		}
	};

	template<typename Allocator>
	auto CreateAllocatorWrapper(Allocator& allocator)
	{
		return AllocatorWrapper<Allocator>{ allocator };
	}

	
	class DefaultAllocatorLabel
	{
		uint64_t value;
	public:
		DefaultAllocatorLabel& operator++() { ++value; return *this; }
		operator uint64_t() const { return value; }
	};


	using DefaultAllocator = ThreadedLabeledBlockAllocator<DefaultAllocatorLabel, 2 * 1024 * 1024, MaxNumThreads>;
	
}
