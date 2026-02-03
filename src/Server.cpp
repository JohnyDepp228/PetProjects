#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <mutex>
#include <semaphore>



void WriteTo(HANDLE hPipe) {
	BOOL write_pipe;
	DWORD written_Bytes = 0;
	char card_num[] = { "1234567812345678\0" };
	write_pipe = WriteFile(hPipe, card_num, 17, &written_Bytes, NULL);
	if (written_Bytes != 17) {
		std::cout << "Written less bytes " << written_Bytes << std::endl;
		exit(1);
	}
	else {
		std::cout << "Success" << std::endl;
	}
}
void Readfrom(HANDLE hPipe) {
	BOOL read_pipe;
	DWORD read_Bytes = 0;
	char card_num1[17] = { 0 };
	std::cout << "Reading form pipe..." << std::endl;
	read_pipe = ReadFile(hPipe, card_num1, 17, &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes != 17) {
			std::cout << "Read less bytes " << read_Bytes << " " << GetLastError() << std::endl;
			exit(1);
		}
		else {
			std::cout << "Success" << std::endl;
		}
		std::cout << "New number of card: " << card_num1 << std::endl;
	}
}
void WriteThread(HANDLE hThread, HANDLE hPipe, DWORD id, HANDLE hSemWr) {
	hThread = CreateThread(
		NULL,
		0,
		[](LPVOID param) -> DWORD {
			HANDLE hPipe = (HANDLE)param;
			WriteTo(hPipe);
			return 0;
		}, hPipe, 0, &id);
	if (hThread == NULL) { std::cout << "Error with thread " << GetLastError() << std::endl; }
	Sleep(500);
	std::cout << "Thread with id " << id << " start writing" << std::endl;
	Sleep(700);
	if (hThread != NULL) { WaitForSingleObject(hThread, INFINITE); }
	if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
	std::cout << "Thread with id " << id << " end" << std::endl;
	if (hThread != NULL) CloseHandle(hThread);
	Sleep(500);
	if (hSemWr != NULL) { ReleaseSemaphore(hSemWr, 1, NULL); }
	std::cout << "SemWr++" << std::endl;
}
int main()
{
	int i = 0;
	BOOL connect_pipe;
	BOOL FlushFile;
	BOOL disconect_pipe;
	BOOL closehandle;
	BOOL CloseHandleThreadWrite;
	HANDLE hPipe = NULL;
	HANDLE hSemWr = NULL;
	HANDLE hSemSigToWrite = NULL;
	HANDLE hSemRd = NULL;
	DWORD id = 0;
	DWORD p_input = 20;
	DWORD p_output = 20;
	HANDLE hThread = NULL;
	HANDLE hThreadReading = NULL;
	HANDLE hThreadWriting = NULL;

	//initialize
	hSemSigToWrite = CreateSemaphoreA(NULL, 0, 1, "SemSigToWrite");
	hSemWr = CreateSemaphoreA(NULL, 0, 1, "MySemWr");
	hSemRd = OpenSemaphoreA(SYNCHRONIZE, FALSE, "MySemRd");
	hPipe = CreateNamedPipeA(
		"\\\\.\\pipe\\Server_pipe"
		, PIPE_ACCESS_DUPLEX
		, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS
		, PIPE_UNLIMITED_INSTANCES
		, p_output
		, p_input
		, NMPWAIT_WAIT_FOREVER
		, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cout << "Error with pipe " << GetLastError() << std::endl;
		return 0;
	}
	else {
		std::cout << "========= Server start =========" << std::endl;
	}


	//connecting
	connect_pipe = ConnectNamedPipe(hPipe, NULL);
	if (connect_pipe) {
		while (1) {
			Sleep(500);
			std::cout << "Working..." << std::endl;


			//signal to write
			hThreadWriting = CreateThread(NULL,
				0,
				[](LPVOID param) -> DWORD {
					HANDLE hSemSigToWrite = (HANDLE)param;
					WaitForSingleObject(hSemSigToWrite, INFINITE);
					//WriteThread(hThread, hPipe, id, hSemWr);
					return 0;
				},
				hSemSigToWrite, 0, NULL);

			//writing
			/*hThread = CreateThread(
				NULL,
				0,
				[](LPVOID param) -> DWORD {
					HANDLE hPipe = (HANDLE)param;
					WriteTo(hPipe);
					return 0;
				}, hPipe, 0, &id);
			if (hThread == NULL) { std::cout << "Error with thread " << GetLastError() << std::endl; }
			Sleep(500);
			std::cout << "Thread with id " << id << " start writing" << std::endl;
			Sleep(700);
			if (hThread != NULL) { WaitForSingleObject(hThread, INFINITE); }
			if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
			std::cout << "Thread with id " << id << " end" << std::endl;
			if (hThread != NULL) CloseHandle(hThread);
			Sleep(500);
			if (hSemWr != NULL) { ReleaseSemaphore(hSemWr, 1, NULL); }
			std::cout << "SemWr++" << std::endl;*/


			//reading 
			/*Sleep(500);
			if (hSemRd != NULL) WaitForSingleObject(hSemRd, INFINITE);
			std::cout << "SemRd--" << std::endl;
			hThread = CreateThread(
				NULL,
				0,
				[](LPVOID param) -> DWORD {
					HANDLE hPipe = (HANDLE)param;
					Readfrom(hPipe);
					return 0;
				}, hPipe, 0, &id);
			Sleep(500);
			std::cout << "Thread with id " << id << " start reading" << std::endl;
			Sleep(700);
			if (hThread != NULL) { WaitForSingleObject(hThread, INFINITE); }
			std::cout << "Thread with id " << id << " end" << std::endl;
			if (hThread != NULL) CloseHandle(hThread);
			Sleep(500);*/
		}
	}
	else {
		std::cout << "Error with  connecting to PIPE " << GetLastError() << std::endl;
	}
	if (hThreadWriting != NULL) WaitForSingleObject(hThreadWriting, INFINITE);
	if (hThreadWriting != NULL) CloseHandleThreadWrite = CloseHandle(hThreadWriting);
	FlushFile = FlushFileBuffers(hPipe);
	disconect_pipe = DisconnectNamedPipe(hPipe);
	closehandle = CloseHandle(hPipe);
	return 0;
}
