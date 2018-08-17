
#ifndef _CUSTOM_ALLOCATOR_H_
#define _CUSTOM_ALLOCATOR_H_

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
//----------------------------------------------------------------------------

#define ALLOCATOR_DEBUGGER

void * __cdecl CustomAllocator_New   (size_t aSize,  int aBlockUse = _NORMAL_BLOCK, char const * aFileName = __FILE__, int aLineNumber = 0);
void   __cdecl CustomAllocator_Delete(void * aBlock, int aBlockUse = _NORMAL_BLOCK, char const * aFileName = __FILE__, int aLineNumber = 0) noexcept;

void * __cdecl CustomAllocator_Malloc(size_t aSize,  int aBlockUse, char const * aFileName, int aLineNumber);
void __cdecl   CustomAllocator_Free  (void * aBlock, int aBlockUse, char const * aFileName, int aLineNumber);

size_t __cdecl memoryUsage();

void _cdecl beginSnapShot();
bool _cdecl endSnapShot();

size_t __cdecl maxAvailable();
double metricFragmentation(int index);

#ifdef ALLOCATOR_DEBUGGER
class __declspec(dllexport) AllocatorEventDebugger
{
public:

	void virtual onNewBlock(void *startBlockAddress, size_t length) = 0;

	void virtual onNewMemory(void *startBlockAddress, void *address, size_t length) = 0;

	void virtual onDeleteMemory(void *startBlockAddress, void *address, size_t length) = 0;

};
void __declspec(dllexport) setEventDebugger(AllocatorEventDebugger* eventDebugger);
#endif
//----------------------------------------------------------------------------

#endif  // _CUSTOM_ALLOCATOR_H_
