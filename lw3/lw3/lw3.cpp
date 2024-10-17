#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>

const int THREADS_AMOUNT = 2;

struct ThreadParams
{
	int threadNum;
	std::ofstream& output;
	DWORD time;
};

DWORD WINAPI ThreadProc(LPVOID params)
{
	ThreadParams threadParams = *(ThreadParams*)params;

	for (int i = 0; i < 20; i++)
	{
		//double g = 3.14 * 3.14 * 3.14;
		threadParams.output << threadParams.threadNum << timeGetTime() - threadParams.time << std::endl;
	}
	ExitThread(0);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: <output1.txt> <output2.txt>" << std::endl;
		return 1;
	}
	DWORD timeStart = timeGetTime();
	HANDLE* threads = new HANDLE[THREADS_AMOUNT];

	for (int i = 0; i < THREADS_AMOUNT; i++)
	{
		std::ofstream output(argv[i]);
		ThreadParams* params = new ThreadParams{ i, output, timeStart };
		LPVOID lpData;
		lpData = (LPVOID)params;
		threads[i] = CreateThread(NULL, 0, &ThreadProc, lpData, CREATE_SUSPENDED, NULL);
	}

	for (int i = 0; i < THREADS_AMOUNT; i++)
		ResumeThread(threads[i]);

	WaitForMultipleObjects(THREADS_AMOUNT, threads, true, INFINITE);
	return 0;
}