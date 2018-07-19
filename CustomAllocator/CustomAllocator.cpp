
#include "stdafx.h"
#include "CustomAllocator.h"

#define MAX_MEMORY 100'000'000

//----------------------------------------------------------------------------

struct address_compare {
	bool operator() (const std::pair<size_t,void*>& lhs, const std::pair<size_t,void*>& rhs) const {
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
	static bool memoryCreated = false;
  // default CRT implementation
	void *ptrMem = nullptr;

	if (memoryCreated == false)
	{
		ptrMem = GlobalAlloc(GMEM_FIXED, (size_t)MAX_MEMORY);
		memoryCreated = true;

		*((size_t*)ptrMem) = aSize;

		startingAddresses.insert({ MAX_MEMORY, (char*)ptrMem + sizeof(size_t) + aSize });

		return (char*)ptrMem + sizeof(size_t);
	}

	auto iteratorAddress= startingAddresses.lower_bound({aSize + sizeof(size_t), (void*)0});

	if (iteratorAddress == startingAddresses.end())
		return ptrMem;

	auto[lengthAddress, pointerAddress] = *iteratorAddress;

	startingAddresses.erase(iteratorAddress);

	if (lengthAddress - sizeof(size_t) - aSize > 0)
		startingAddresses.insert({ lengthAddress - sizeof(size_t) - aSize , (char*)pointerAddress + sizeof(size_t) + aSize });

	*((size_t*)pointerAddress) = aSize;

	ptrMem = (char*)pointerAddress + sizeof(size_t);

	return ptrMem;

  //return _malloc_dbg(aSize, aBlockUse, aFileName, aLineNumber);
}

void __cdecl CustomAllocator_Free(void * aBlock, int /*aBlockUse*/, char const * /*aFileName*/, int /*aLineNumber*/)
{
  // default CRT implementation
	GlobalFree(aBlock);
}

