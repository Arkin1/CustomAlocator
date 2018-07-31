
#include "stdafx.h"

using namespace std;


char* p[2'500'000];

int main()
{

	//memoryVisualise();

	srand(53545);

	int k = 0;

	double mean_time_new = 0;
	long long nr_new=0;
	double mean_time_delete = 0;
	long long nr_delete = 0;


	while (true)
	{

		if (rand() % 5 > 1)
		{
			if (rand() % 31 == 0)
			{
				int x = rand() + 1;
				auto start_time =std::chrono::high_resolution_clock::now();
				p[k] = new char[x];
				mean_time_new+= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			}
			else
			{
				int x = rand() % 10 + 1;
				auto start_time = std::chrono::high_resolution_clock::now();
				p[k] = new char[x];
				mean_time_new+= std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			}

			nr_new++;

			if (p[k] != 0)
			{
				++k;
			}
			else
			{
				break;

			}


		}
		else if (k != 0)
		{
			nr_delete++;
			int x = rand() % (k/2) + k/2;

			auto start_time = std::chrono::high_resolution_clock::now();
			delete[] p[x];
			mean_time_delete += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			
			for (int j = x + 1; j < k; ++j)
			{
				p[j - 1] = p[j];
			}
			--k;
		}

		cout << "memory usage:" <<  memoryUsage()  << '\n';

		cout << "max available space:" << maxAvailable() << " bytes          \n";

		cout << "matrix fragmentation:" << metricFragmentation() << "            \n";



		COORD coord;

		coord.X = 0;

		coord.Y = 0;

		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	

	}


	cout << "new operation mean time: " << mean_time_new / nr_new << " ms\n";
	cout << "delete operation mean time: " << mean_time_delete / nr_delete << " ms\n";

	system("pause");

  return 0;
}

