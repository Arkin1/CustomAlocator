
#include "stdafx.h"
#include "CustomAllocator.h"

#define MAX_MEMORY 1000000

//----------------------------------------------------------------------------

void *startMemAddress;

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

std::map<char*, size_t> occupiedAddresses;

std::map<char*, size_t> snapShotOccupiedAddresses;

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
		startMemAddress = ptrMem;

		memoryCreated = true;

		//memset((char*)ptrMem, 0, (size_t)MAX_MEMORY);


		startingAddresses.insert({ MAX_MEMORY - aSize, (char*)ptrMem  + aSize });

		memset(ptrMem, 0, MAX_MEMORY);

		occupiedAddresses[(char*)ptrMem] = aSize;

		return (char*)ptrMem;
	}

	auto iteratorAddress= startingAddresses.lower_bound({aSize, (void*)0});

	if (iteratorAddress == startingAddresses.end())
	{
		return ptrMem; //nullptr
	}

	auto[lengthAddress, pointerAddress] = *iteratorAddress;
	startingAddresses.erase(iteratorAddress);

	if (lengthAddress - aSize > 0)
	{
		startingAddresses.insert({ lengthAddress - aSize , (char*)pointerAddress  + aSize });
	}
	

	ptrMem = (char*)pointerAddress;

	occupiedAddresses[(char*)ptrMem] = aSize;

	return ptrMem;

  //return _malloc_dbg(aSize, aBlockUse, aFileName, aLineNumber);
}

void __cdecl CustomAllocator_Free(void * aBlock, int /*aBlockUse*/, char const * /*aFileName*/, int /*aLineNumber*/)
{
	auto location = occupiedAddresses.find((char*)aBlock);
	
	if (location == end(occupiedAddresses))
	{
		printf("Double free exception.\n");
		return;
	}
	size_t length =(*location).second ;
	auto prev_loc = location;
	void *startAddress = nullptr;
	location++;
	if (location != end(occupiedAddresses))
	{
		startingAddresses.erase({ (*location).first - ((*prev_loc).first + (*prev_loc).second), (*location).first + (*location).second });
		length += (*location).first - ((*prev_loc).first + (*prev_loc).second);
	}
	else
	{
		length += MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second);
		if(MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) !=0)
			startingAddresses.erase({ MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) ,
								(*prev_loc).first + (*prev_loc).second });
	}
	
	location--;
	
	if (location != begin(occupiedAddresses))
	{
		location--;

		startingAddresses.erase({ (*prev_loc).first - ((*location).first + (*location).second), ((*location).first + (*location).second)  });
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

	//memset((char*)aBlock - sizeof(size_t), 0, sizeof(size_t) + *(((size_t*)aBlock) - 1));



	// default CRT implementation
	// GlobalFree(aBlock);
}

void _cdecl memoryVisualise()
{
	//Get a console handle
	HWND myconsole = GetConsoleWindow();
	//Get a handle to device context
	HDC mydc = GetDC(myconsole);

	//Choose any color
	COLORREF COLOR = RGB(0, 255, 255);

	const int MAX_SQRT = (int)sqrt(MAX_MEMORY);

	//Draw pixels
	for (int i = 0, j = 50; i < MAX_MEMORY; i++)
	{
		SetPixel(mydc, i % MAX_SQRT, j, COLOR);
		j += ( i % MAX_SQRT == 0);
	}
	
	COLOR = RGB(255, 0, 0);

	for (const auto&[key, length] : occupiedAddresses)
	{
		for (size_t i = key - (char*)startMemAddress, j = (size_t)(i / MAX_SQRT) + 50; i < key - (char*)startMemAddress + length; i++)
		{
			SetPixel(mydc,(i + 1) % MAX_SQRT, (int)j, COLOR);
			j += ((i + 1) % MAX_SQRT == 0);
		}
	}

	ReleaseDC(myconsole, mydc);
}

void _cdecl beginSnapShot()
{
	//snapShotOccupiedAddresses.clear();

	snapShotOccupiedAddresses.insert(begin(occupiedAddresses), end(occupiedAddresses));
}

bool _cdecl endSnapShot()
{
	for (auto [key,value] : occupiedAddresses)
	{
		if (snapShotOccupiedAddresses.find(key) == snapShotOccupiedAddresses.end())
			return 1;
	}
	return 0;
}

void _cdecl memoryUsage()
{
	double sum = 0;
	for (const auto&[key, length] : occupiedAddresses)
	{
		sum += length;
	}
	std::cout << std::setprecision(6) << std::fixed << "Memory used: %" << (sum / MAX_MEMORY * 100) << '\n';
}

