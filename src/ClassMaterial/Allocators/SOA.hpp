#pragma once

#include <cstring>
#include <tuple>

namespace Utilities
{
	template<typename T>
	struct PassThroughHasher
	{
		T operator()(const T& t) { return t; }
	};

	template<typename K, typename T, typename KeyHasher = PassThroughHasher<K>>
	struct HashMapBase
	{
	public:
		static constexpr size_t ComputeActiveElementsSize(size_t numElements)
		{
			return (((numElements * 2) - 1) / 64) + 1;
		}

		int GetNumActiveElements() const { return numActiveElements; }

		void Set(K name, const T& data)
		{
			size_t index = GetHashIndex(name);
			if (!IsActive(index))
			{
				++numActiveElements;
				SetActive(index);
			}
			namesTable[index] = name;
			payload[index] = data;
		}

		struct ConstIterator
		{
			ConstIterator& operator++()
			{
				do
				{
					++index;
				} while (index < (int)hashMap->tableSize && !hashMap->IsActive(index));
				return *this;
			}

			std::tuple<K, const T&> operator*() const
			{
				return { hashMap->namesTable[index], hashMap->payload[index] };
			}

			bool operator==(const ConstIterator& other) { return index == other.index && hashMap == other.hashMap; }
			bool operator!=(const ConstIterator& other) { return index != other.index || hashMap != other.hashMap; }

			const HashMapBase *hashMap;
			int index;
			ConstIterator(const HashMapBase *_hashMap, int _index) : hashMap(_hashMap), index(_index) {};
		};

		ConstIterator begin() const
		{
			int index = 0;
			while (index < (int)tableSize && !IsActive(index))
			{
				++index;
			}
			return ConstIterator(this, index);
		}
		ConstIterator end() const
		{
			return ConstIterator(this, tableSize);
		}

		struct Payload
		{
			T* data;
			size_t index;
			operator T*() const { return data; }
			T* operator->() const { return data; }
			T& operator*() const { return *data; }
			bool operator==(nullptr_t) { return data == nullptr; }
			bool operator!=(nullptr_t) { return data != nullptr; }
		};

		Payload Reserve(K name)
		{
			assert(numActiveElements < tableSize * 4 / 5);
			size_t index = GetHashIndex(name);
			assert(!IsActive(index));
			++numActiveElements;
			SetActive(index);
			namesTable[index] = name;
			return{ &payload[index], index };
		}

		const Payload Get(K name) const
		{
			size_t index = GetHashIndex(name);
			return{ IsActive(index) ? &payload[index] : nullptr, index };
		}
		Payload Get(K name)
		{
			size_t index = GetHashIndex(name);
			return{ IsActive(index) ? &payload[index] : nullptr, index };
		}

		void Delete(K name)
		{
			size_t index = GetHashIndex(name);
			assert(isActive(index));
			SetDeleted(index);
		}

		float Occupancy() const
		{
			return (float)numActiveElements / (float)tableSize;
		}

	protected:
		HashMapBase(K *_namesTable, T *_payload, uint64_t *_activeElements, uint32_t _tableSize)
			: namesTable(_namesTable)
			, payload(_payload)
			, activeElements(_activeElements)
			, tableSize(_tableSize)
			, numActiveElements(0)
		{ 
			memset(activeElements, 0, ComputeActiveElementsSize(tableSize) * sizeof(uint64_t));
		}

	private:

		bool IsActive(size_t index) const
		{
			return (activeElements[(index * 2) / 64] & (1ULL << ((index * 2) % 64))) != 0;
		}
		bool IsDeleted(size_t index) const
		{
			return (activeElements[(index * 2 + 1) / 64] & (1ULL << ((index * 2 + 1) % 64))) != 0;
		}
		void SetActive(size_t index) const
		{
			assert(!IsActive(index));
			activeElements[(index * 2) / 64] |= (1ULL << ((index * 2) % 64));
			activeElements[(index * 2 + 1) / 64] &= ~(1ULL << ((index * 2 + 1) % 64));
		}
		void SetDeleted(size_t index) const
		{
			assert(!IsActive(index));
			activeElements[(index * 2) / 64] &= ~(1ULL << ((index * 2) % 64));
			activeElements[(index * 2 + 1) / 64] |= (1ULL << ((index * 2 + 1) % 64));
		}

		size_t GetHashIndex(K name) const
		{
			KeyHasher hasher;
			uint_fast16_t index = (uint_fast16_t)(hasher(name) % tableSize);

			while ((IsActive(index) && namesTable[index] != name) || IsDeleted(index))
			{
				index = (uint_fast16_t)((index + 1) % tableSize);
			}
			return (size_t)index;
		}

		K *namesTable;
		T *payload;
		uint64_t *activeElements;
		uint32_t tableSize;
		uint32_t numActiveElements;
	};


	template<typename K, typename T, uint32_t Size, typename KeyHasher = PassThroughHasher<K>>
	struct HashMap : HashMapBase<K, T, KeyHasher>
	{
	public:
		HashMap() : HashMapBase<K, T, KeyHasher>(namesTable, payload, activeElements, Size) {}

	private:
		K namesTable[Size];
		T payload[Size];
		uint64_t activeElements[ComputeActiveElementsSize(Size)];
	};

	// struct HashedStringHasher {
	// 	uint32_t operator()(HashedString hs) { return hs.value; };
	// };
    // 
	// template<typename T, uint32_t Size>
	// using HashedStringHashMap = HashMap<HashedString, T, Size, HashedStringHasher>;
	// template<typename T>
	// using HashedStringHashMapBase = HashMapBase<HashedString, T, HashedStringHasher>;

	// ------------------------------------------------------------------------------------------------

	struct StackAllocator;

	template<typename T>
	struct ResizableArray
	{
		T* ptr;
		size_t capacity, size;

		ResizableArray(size_t initialCapacity, StackAllocator& allocator)
			: ptr(allocator.alloc<T>(initialCapacity))
			, capacity(initialCapacity)
			, size(0)
		{}

		T& operator[](size_t index) { return ptr[index]; }
		const T& operator[](size_t index) const { return ptr[index]; }

		void Resize(size_t newCapacity, StackAllocator& allocator)
		{
			if (newCapacity > capacity)
			{
				T* aux = allocator.alloc<T>(newCapacity);
				for (int i = 0; i < (int)size; ++i)
					aux[i] = ptr[i];
				ptr = aux;
			}
			capacity = newCapacity;
		}

		void Push(const T& element, StackAllocator& allocator)
		{
			if (size == capacity)
			{
				Resize(capacity * 2, allocator);
			}
			assert(size < capacity);
			ptr[size] = element;
			++size;
		}
	};
}
