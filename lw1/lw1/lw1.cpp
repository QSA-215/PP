#include <windows.h>
#include <string>
#include <iostream>

DWORD WINAPI ThreadProc(CONST LPVOID param)
{
	int threadNum = *(int*)param; 
	std::cout << "Thread number " << threadNum << " is working\n";

	ExitThread(0); // функция устанавливает код завершения потока в 0
}

int main()
{
	int threadAmount;
	std::cin >> threadAmount;

	// создание потоков
	HANDLE* handles = new HANDLE[threadAmount];

	for (int i = 0; i < threadAmount; i++)
	{
		int* number = new int{i};
		LPVOID lpData;
		lpData = (LPVOID)number;

		handles[i] = CreateThread(NULL, 0, &ThreadProc, lpData, CREATE_SUSPENDED, NULL);
	}

	// запуск потоков
	for (int i = 0; i < threadAmount; i++)
	{
		ResumeThread(handles[i]);
	}

	// ожидание окончания работы потоков
	WaitForMultipleObjects(threadAmount, handles, true, INFINITE);
	return 0;
}