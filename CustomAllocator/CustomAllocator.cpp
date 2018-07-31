#include "stdafx.h"
#include "CustomAllocator.h"

#define MAX_MEMORY 2000
#define MAX_NUMBER_MEM_BLOCKS 500
//----------------------------------------------------------------------------

int consoleWidth;
int nextX = 0;
int nextY = 0;

class MemoryBlock
{
private:

	bool memoryCreated;
	void *startMemAddress;

	static HWND console;
	static HDC hdc;

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

	int xLeft, yLeft;


	void drawRange(char* key, size_t length, COLORREF COLOR)
	{
		const int MAX_SQRT = (int)sqrt(MAX_MEMORY);

		if ((int)length < MAX_SQRT)
		{
			for (size_t i = key - (char*)startMemAddress, j = (size_t)(i / MAX_SQRT) + yLeft;
				i < key - (char*)startMemAddress + length; i++)
			{
				SetPixel(hdc, xLeft  + (i + 1) % MAX_SQRT, (int)j, COLOR);
				j += ((i + 1) % MAX_SQRT == 0);
			}
		}
		else
		{
			size_t i, j;
			for (i = key - (char*)startMemAddress, j = (size_t)(i / MAX_SQRT) + yLeft;
				(i  % MAX_SQRT) != 0; i++)
			{
				SetPixel(hdc, xLeft + ((int)i + 1) % MAX_SQRT, (int)j, COLOR);
				length--;
			}
			j++;
			RECT myRect = { xLeft, (int)j, xLeft + MAX_SQRT, (int)j + (int)length / MAX_SQRT };
			j += (int)length / MAX_SQRT;
			length -= MAX_SQRT * ((int)length / MAX_SQRT) - 1;
			HBRUSH handler = CreateSolidBrush(COLOR);
			FillRect(hdc, &myRect, handler);
			DeleteObject(handler);

			int k = 1;
			while (length--)
			{
				SetPixel(hdc, xLeft + k++, (int)j, COLOR);
			}
		}
	}

	void clearVisualiserArea()
	{
		const int MAX_SQRT = (int)sqrt(MAX_MEMORY);
		COLORREF COLOR = RGB(0, 139, 139);

		//Draw pixels
		RECT myRect = { xLeft, yLeft,xLeft +  MAX_SQRT, MAX_SQRT + yLeft + 1 };
		HBRUSH handler = CreateSolidBrush(COLOR);
		FillRect(hdc, &myRect, handler);

		DeleteObject(handler);

		COLOR = RGB(255, 255, 102);

		myRect = { xLeft + MAX_SQRT , yLeft,xLeft + MAX_SQRT + 1, MAX_SQRT + yLeft + 1 };

		handler = CreateSolidBrush(COLOR);
		FillRect(hdc, &myRect, handler);

		DeleteObject(handler);

		COLOR = RGB(255, 255, 102);

		myRect = { xLeft ,  yLeft + MAX_SQRT + 1,xLeft + MAX_SQRT + 1, yLeft + MAX_SQRT + 2 };

		handler = CreateSolidBrush(COLOR);
		FillRect(hdc, &myRect, handler);

		DeleteObject(handler);

		COLOR = RGB(255, 255, 102);

		myRect = { xLeft ,  yLeft - 1,xLeft + MAX_SQRT + 1, yLeft + 1 };

		handler = CreateSolidBrush(COLOR);
		FillRect(hdc, &myRect, handler);

		DeleteObject(handler);

		COLOR = RGB(255, 255, 102);

		myRect = { xLeft-1 , yLeft,xLeft, yLeft + MAX_SQRT + 1 };

		handler = CreateSolidBrush(COLOR);
		FillRect(hdc, &myRect, handler);

		DeleteObject(handler);

	}

public:

	MemoryBlock() : memoryCreated(false), xLeft(1), yLeft(0) {}

	void setVisualiserCoordinates(int x, int y)
	{
		xLeft = x;
		yLeft = y;

		if (hdc != 0)
			clearVisualiserArea();
	}

	void* malloc(size_t aSize)
	{
		// default CRT implementation
		void *ptrMem = nullptr;

		if (memoryCreated == false)
		{
			ptrMem = GlobalAlloc(GMEM_FIXED, (size_t)MAX_MEMORY);
			startMemAddress = ptrMem;

			memoryCreated = true;

			//memset((char*)ptrMem, 0, (size_t)MAX_MEMORY);


			startingAddresses.insert({ MAX_MEMORY - aSize, (char*)ptrMem + aSize });

			memset(ptrMem, 0, MAX_MEMORY);

			occupiedAddresses[(char*)ptrMem] = aSize;

			if (hdc != 0)
				drawRange((char*)ptrMem, aSize, RGB(139, 0, 0));


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

		if (hdc != 0)
			drawRange((char*)ptrMem, aSize, RGB(139, 0, 0));

		return ptrMem;
	}

	bool free(void *aBlock)
	{
		auto location = occupiedAddresses.find((char*)aBlock);



		if (location == end(occupiedAddresses))
		{
			return false;
		}
		size_t length = (*location).second;

		if (hdc != 0)
			drawRange((*location).first, length, RGB(0, 139, 139));

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
			length += MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second);
			if (MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) != 0)
				startingAddresses.erase({ MAX_MEMORY - ((*prev_loc).first - (char*)startMemAddress + (*prev_loc).second) ,
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
		//std::cout << std::setprecision(6) << std::fixed << "Memory used: %" << (sum / MAX_MEMORY * 100) << '\n';
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

		return 1 - (4 * pow(((long long)startingAddresses.size() - (long long)MAX_MEMORY / 2), 2) / (double)MAX_MEMORY / (double)MAX_MEMORY)*
			((double)memory_Usage() / (MAX_MEMORY - max_Available()));

	}

	friend void memoryVisualise();

};

HWND MemoryBlock::console = 0;
HDC MemoryBlock ::hdc = 0;



std:: list<MemoryBlock> memoryBlocks;

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
	for (auto& memBlock : memoryBlocks)
	{
		void *memAddress = memBlock.malloc(aSize);

		if (memAddress != nullptr)
			return memAddress;

	}

	if (memoryBlocks.size() < MAX_NUMBER_MEM_BLOCKS)
	{
		memoryBlocks.push_back(MemoryBlock());


		if (nextX + sqrt(MAX_MEMORY) + 1 < consoleWidth)
		{
			memoryBlocks.back().setVisualiserCoordinates(nextX + 1, nextY + 50);

			nextX += (int)(sqrt(MAX_MEMORY) + 1) + 1;
		}
		else
		{
			nextX = 0;
			nextY += (int)(sqrt(MAX_MEMORY)) + 3;
			memoryBlocks.back().setVisualiserCoordinates(nextX + 1, nextY + 50);
			nextX += (int)(sqrt(MAX_MEMORY) + 1) + 1;

		}


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
void _cdecl memoryVisualise()
{
	//Get a console handle
	MemoryBlock:: console = GetConsoleWindow();
	//Get a handle to device context
	MemoryBlock::hdc = GetDC(MemoryBlock::console);


	RECT rect;

	GetWindowRect(MemoryBlock::console, &rect);


	consoleWidth = rect.right;
	
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
	if (index < memoryBlocks.size())
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