
#include "stdafx.h"
#include "CustomAllocator.h"

#define MAX_MEMORY 10e8

//----------------------------------------------------------------------------

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
	static void* next = nullptr;
	void *ptrMem = nullptr;

	if (memoryCreated == false)
	{
		ptrMem = GlobalAlloc(GMEM_FIXED, (size_t)MAX_MEMORY);
		memoryCreated = true;

		next = (char*)ptrMem + aSize;

		return ptrMem;
	}
	
	ptrMem = next;

	next = (char*)ptrMem + aSize;
	

	return ptrMem;

  //return _malloc_dbg(aSize, aBlockUse, aFileName, aLineNumber);
}

void __cdecl CustomAllocator_Free(void * aBlock, int /*aBlockUse*/, char const * /*aFileName*/, int /*aLineNumber*/)
{
  // default CRT implementation
	GlobalFree(aBlock);
}

