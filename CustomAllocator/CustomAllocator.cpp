#include "stdafx.h"
#include "CustomAllocator.h"



#define MAX_NUMBER_MEM_BLOCKS 60
//----------------------------------------------------------------------------

#ifdef ALLOCATOR_DEBUGGER
	static AllocatorEventDebugger *allocEventDebugger = nullptr;

	struct BlockParams
	{
		void *startBlockAddress;
		size_t mem;
	};

	enum class MemTypeEv : int
	{
		MallocEvent,
		FreeEvent
	};

	struct MemParams
	{
		MemTypeEv type;
		void *startBlockAddress;
		void *startMemAddress;
		size_t mem;
	};

	static std::vector< std::variant<BlockParams, MemParams> > notCatchedEvents;

#endif

class MemoryBlock
{
private:

	int Memory;

	bool memoryCreated;
	void *startMemAddress;

	struct address_compare {
		bool operator() (const std::pair<size_t, void*>& lhs, const std::pair<size_t, void*>& rhs) const {
			auto[length_pr1, pointer_pr1] = lhs;
			auto[length_pr2, pointer_pr2] = rhs;

			if (length_pr1 == length_pr2)
			{
				return (char*)pointer_pr1 < (char*)pointer_pr2;
			}
			else
				return length_pr1 < length_pr2;
		}
	};

	std::multiset < std::pair<size_t, void*>, address_compare> startingAddresses;

	std::map<char*, size_t> occupiedAddresses;

	std::map<char*, size_t> snapShotOccupiedAddresses;



public:

	MemoryBlock(int mem) : memoryCreated(false),Memory(mem) {}



	void* malloc(size_t aSize)
	{
		// default CRT implementation
		void *ptrMem = nullptr;

		if (memoryCreated == false)
		{
			ptrMem = GlobalAlloc(GMEM_FIXED, (size_t)Memory);
			startMemAddress = ptrMem;

			memoryCreated = true;

			//memset((char*)ptrMem, 0, (size_t)Memory);


			startingAddresses.insert({ Memory - aSize, (char*)ptrMem + aSize });

			memset(ptrMem, 0, Memory);

			occupiedAddresses[(char*)ptrMem] = aSize;

			#ifdef ALLOCATOR_DEBUGGER
				
				if (allocEventDebugger != nullptr)
				{
					allocEventDebugger->onNewBlock(startMemAddress, (size_t)Memory);
					allocEventDebugger->onNewMemory(startMemAddress, ptrMem, aSize);
				}
				else
				{
					BlockParams blockParams;
					blockParams.startBlockAddress= startMemAddress;
					blockParams.mem = (size_t)Memory;

					notCatchedEvents.push_back(std::variant<BlockParams, MemParams>(blockParams));

					MemParams memParams;

					memParams.type = MemTypeEv::MallocEvent;
					memParams.startBlockAddress = startMemAddress;
					memParams.startMemAddress = ptrMem;
					memParams.mem = aSize;

					notCatchedEvents.push_back(std::variant<BlockParams, MemParams>(memParams));
				}
			#endif


			return (char*)ptrMem;
		}

		auto iteratorAddress = startingAddresses.lower_bound({ aSize, (void*)0 });

		if (iteratorAddress == startingAddresses.end())
		{
			return ptrMem; //nullptr
		}

		auto[lengthAddress, pointerAddress] = *iteratorAddress;
		startingAddresses.erase(iteratorAddress);

		if (lengthAddress - aSize > 0)
		{
			startingAddresses.insert({ lengthAddress - aSize , (char*)pointerAddress + aSize });
		}


		ptrMem = (char*)pointerAddress;

		occupiedAddresses[(char*)ptrMem] = aSize;

		#ifdef ALLOCATOR_DEBUGGER
			if (allocEventDebugger != nullptr)
			{
				allocEventDebugger->onNewMemory(startMemAddress, ptrMem, aSize);
			}
			else
			{
				MemParams memParams;

				memParams.type = MemTypeEv::MallocEvent;
				memParams.startBlockAddress = startMemAddress;
				memParams.startMemAddress = ptrMem;
				memParams.mem = aSize;

				notCatchedEvents.push_back(std::variant<BlockParams, MemParams>(memParams));
			}
		#endif

		return ptrMem;
	}

	bool free(void *aBlock)
	{
		auto location = occupiedAddresses.find((char*)aBlock);

		Sleep(10);

		if (location == end(occupiedAddresses))
		{
			return false;
		}
		size_t length = (*location).second;

		#ifdef ALLOCATOR_DEBUGGER
			if (allocEventDebugger != nullptr)
			{
				allocEventDebugger->onDeleteMemory(startMemAddress, (*location).first, length);
			}
			else
			{
				MemParams memParams;

				memParams.type = MemTypeEv::FreeEvent;
				memParams.startBlockAddress = startMemAddress;
				memParams.startMemAddress = (*location).first;
				memParams.mem = length;

				notCatchedEvents.push_back(std::variant<BlockParams, MemParams>(memParams));
			}

		#endif

		auto prev_loc = location;
		void *startAddress = nullptr;
		location++;
		if (location != end(occupiedAddresses))
		{
			if ((*location).first - ((*prev_loc).first + (*prev_loc).second) > 0)
				startingAddresses.erase({ (*location).first - ((*prev_loc).first + (*prev_loc).second), (*prev_loc).first + (*prev_loc).second });
			length += (*location).first - ((*prev_loc).first + (*prev_loc).second);
		}
		else
		{
			length += Memory - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second);
			if (Memory - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) != 0)
				startingAddresses.erase({ Memory - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) ,
				(*prev_loc).first + (*prev_loc).second });
		}

		location--;

		if (location != begin(occupiedAddresses))
		{
			location--;
			if ((*prev_loc).first - ((*location).first + (*location).second) > 0)
				startingAddresses.erase({ (*prev_loc).first - ((*location).first + (*location).second), ((*location).first + (*location).second) });
			length += (*prev_loc).first - ((*location).first + (*location).second);
			startAddress = ((*location).first + (*location).second);
		}
		else
		{

			length += (*prev_loc).first - (char*)startMemAddress;
			startAddress = startMemAddress;

			if (length != 0)
			{
				startingAddresses.erase({ (*prev_loc).first - (char*)startMemAddress , startMemAddress });
			}

		}

		occupiedAddresses.erase(prev_loc);


		startingAddresses.insert({ length, startAddress });

		return true;


	}

	void begin_SnapShot()
	{
		//snapShotOccupiedAddresses.clear();

		snapShotOccupiedAddresses.insert(begin(occupiedAddresses), end(occupiedAddresses));
	}

	bool end_SnapShot()
	{
		for (auto[key, value] : occupiedAddresses)
		{
			if (snapShotOccupiedAddresses.find(key) == snapShotOccupiedAddresses.end())
				return 1;
		}
		return 0;
	}

	size_t  memory_Usage()
	{
		size_t sum = 0;
		for (const auto&[key, length] : occupiedAddresses)
		{
			sum += length;
		}

		return sum;
		//std::cout << std::setprecision(6) << std::fixed << "Memory used: %" << (sum / Memory * 100) << '\n';
	}

	size_t max_Available()
	{
		if (startingAddresses.size() > 0)
			return rbegin(startingAddresses)->first;
		else
			return 0;
	}

	double metric_Fragmentation()
	{

		return 1 - (4 * pow(((long long)startingAddresses.size() - (long long)Memory / 2), 2) / (double)Memory / (double)Memory)*
			((double)memory_Usage() / (Memory - max_Available()));

	}


};


static std::list<MemoryBlock> memoryBlocks;

void * __cdecl CustomAllocator_New(size_t aSize, int aBlockUse, char const * aFileName, int aLineNumber)
{
	return CustomAllocator_Malloc(aSize, aBlockUse, aFileName, aLineNumber);
}

void __cdecl CustomAllocator_Delete(void * aBlock, int aBlockUse, char const * aFileName, int aLineNumber) noexcept
{
	CustomAllocator_Free(aBlock, aBlockUse, aFileName, aLineNumber);
}

void * __cdecl CustomAllocator_Malloc(size_t aSize, int/* aBlockUse*/, char const * /*aFileName*/, int /*aLineNumber*/)
{
	const int MAX_MEMORY = 2097152/2;

	const int MEM_DELIM = 102400 / 8;

	const int SPECIAL_BLOCKS_NUMBER = 0;

	if (memoryBlocks.size() == 0)
	{
		for (int i = 0; i < SPECIAL_BLOCKS_NUMBER; ++i)
		{
			memoryBlocks.push_back(MemoryBlock(MEM_DELIM));
		}
	}

	int nr = 0;
	for (auto& memBlock : memoryBlocks)
	{
		nr++;
		if (aSize > 100 && nr <= SPECIAL_BLOCKS_NUMBER) continue;
		
		void *memAddress = memBlock.malloc(aSize);

		if (memAddress != nullptr)
			return memAddress;

	}

	if (memoryBlocks.size() < MAX_NUMBER_MEM_BLOCKS)
	{
		memoryBlocks.push_back(MemoryBlock(MAX_MEMORY));

		return memoryBlocks.back().malloc(aSize);
	}
	else
		return nullptr;

	//return _malloc_dbg(aSize, aBlockUse, aFileName, aLineNumber);
}

void __cdecl CustomAllocator_Free(void * aBlock, int /*aBlockUse*/, char const * /*aFileName*/, int /*aLineNumber*/)
{

	for (auto& memBlock : memoryBlocks)
	{
		memBlock.free(aBlock);
	}
	// default CRT implementation
	// GlobalFree(aBlock);
}

void _cdecl beginSnapShot()
{
	//snapShotOccupiedAddresses.clear();

	for (auto& memBlock : memoryBlocks)
	{
		memBlock.begin_SnapShot();
	}
}

bool _cdecl endSnapShot()
{
	for (auto& memBlock : memoryBlocks)
	{
		if (memBlock.end_SnapShot() == 1)
			return 1;
	}
	return 0;
}

size_t _cdecl memoryUsage()
{
	size_t sum = 0;
	for (auto& memBlock : memoryBlocks)
	{
		sum += memBlock.memory_Usage();
	}


	return sum;
	//std::cout << std::setprecision(6) << std::fixed << "Memory used: %" << (sum / MAX_MEMORY * 100) << '\n';
}

double _cdecl metricFragmentation(int index)
{
	if (index < (int)memoryBlocks.size())
	{
		auto it = begin(memoryBlocks);


		advance(it, index);

		return (*it).metric_Fragmentation();
	}
	else
		return -1;

}

size_t _cdecl maxAvailable()
{
	size_t mx = 0;

	for (auto& memBlock : memoryBlocks)
		mx = max(mx, memBlock.max_Available());

	return mx;
}

#ifdef ALLOCATOR_DEBUGGER
	void _cdecl setEventDebugger(AllocatorEventDebugger* eventDebugger)
	{
		allocEventDebugger = eventDebugger;

		for (const auto &ev : notCatchedEvents)
		{
			if (std::holds_alternative<BlockParams>(ev))
			{
				BlockParams blockParams = std::get<BlockParams>(ev);

				allocEventDebugger->onNewBlock(blockParams.startBlockAddress, blockParams.mem);
			}
			else
			{
				MemParams memParams = std::get<MemParams>(ev);

				if (memParams.type == MemTypeEv::MallocEvent)
				{
					allocEventDebugger->onNewMemory(memParams.startBlockAddress,
													memParams.startMemAddress,
													memParams.mem);
				}
				else
				{
					allocEventDebugger->onNewMemory(memParams.startBlockAddress,
													memParams.startMemAddress,
													memParams.mem);
				}

			}
		}

		notCatchedEvents.clear();

	}
#endif