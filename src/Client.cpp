#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <mutex>

void ReadFromFile(HANDLE hPipe) {
	BOOL read_pipe;
	DWORD read_Bytes = 0;
	char card_num[17] = { 0 };
	read_pipe = ReadFile(hPipe, card_num, 17, &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes > 17) {
			std::cout << "Read less " << GetLastError() << std::endl;
			exit(1);
		}
		else {
			std::cout << "Number of card: " << card_num << std::endl;
		}
	}
	else {
		std::cout << "Error with reading " << GetLastError() << std::endl;
		exit(1);
	}
}

void WritetoFile(HANDLE hPipe) {
	BOOL write_pipe;
	DWORD wrote_Bytes = 0;
	char card_num[] = { "0000000000000000\0" };
	write_pipe = WriteFile(hPipe, card_num, 17, &wrote_Bytes, NULL);
	if (write_pipe) {
		if (wrote_Bytes < 17) {
			std::cout << "Wrote less " << GetLastError() << std::endl;
			exit(1);
		}
		else {
			std::cout << "Number of card: " << card_num << std::endl;
		}
	}
	else {
		std::cout << "Error with writting " << GetLastError() << std::endl;
		exit(1);
	}
}


void ThreadWrite(LPVOID param, HANDLE hSemWr, HANDLE hPipe, HANDLE hSemSigToWrite) {
	ReleaseSemaphore(hSemSigToWrite, 1, NULL);
	if (hSemWr != NULL) WaitForSingleObject(hSemWr, INFINITE);
	std::cout << "SemWr--" << std::endl;
	DWORD id1 = 0;
	HANDLE hThread = CreateThread(NULL, 0,
		[](LPVOID lpParam) -> DWORD {
			HANDLE hPipe = (HANDLE)lpParam;
			ReadFromFile(hPipe);
			return 0;
		}
	, hPipe, 0, &id1);
	Sleep(500);
	std::cout << "Thread with id " << id1 << " start" << std::endl;
	if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
	Sleep(500);
	std::cout << "Thread with id " << id1 << " end" << std::endl;
	if (hThread != NULL) CloseHandle(hThread);
}
int main()
{
	HANDLE hPipe;
	HANDLE hSemRd;
	HANDLE hSemSigToWrite;
	hSemRd = CreateSemaphoreA(NULL, 0, 1, "MySemRd");
	HANDLE hSemWr;
	hSemWr = OpenSemaphoreA(SYNCHRONIZE, FALSE, "MySemWr");
	hSemSigToWrite = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, "SemSigToWrite");
	hPipe = CreateFileA(
		"\\\\.\\pipe\\Server_pipe"
		, GENERIC_ALL
		, FILE_SHARE_WRITE | FILE_SHARE_READ
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cout << "Error with connecting to pipe " << GetLastError() << std::endl;
		exit(1);
	}
	else {
		std::cout << "========= Client start =========" << std::endl;
	}

	DWORD p_input = 20;
	DWORD p_output = 20;
	char choose;
	int i = 0;
	while (1) {
		std::cout << "Waiting..." << std::endl;
		//server -> read
		std::cin >> choose;
		switch (choose) {
		case '1':
			/*ThreadWrite(hPipe, hSemWr, hPipe, hSemSigToWrite);*/
			ReleaseSemaphore(hSemSigToWrite, 1, NULL); break;
		case '2':return 2; break;

		}



		//Write->server
		/*DWORD id2 = 0;
		HANDLE hThread1 = CreateThread(NULL, 0,
			[](LPVOID lpParam) -> DWORD {
				HANDLE hPipe = (HANDLE)lpParam;
				WritetoFile(hPipe);
				return 0;
			}
		, hPipe, 0, &id2);
		Sleep(500);
		std::cout << "Thread with id " << id2 << " start" << std::endl;
		if (hThread1 != NULL) WaitForSingleObject(hThread1, INFINITE);
		std::cout << "Thread with id " << id2 << " end" << std::endl;
		if (hThread1 != NULL) CloseHandle(hThread1);
		if (hSemRd != NULL) ReleaseSemaphore(hSemRd, 1, NULL);
		std::cout << "SemRd++" << std::endl;
		Sleep(700);*/
	}
	CloseHandle(hPipe);
	return 0;
}
