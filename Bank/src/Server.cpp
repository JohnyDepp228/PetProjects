#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <mutex>
#include <semaphore>
#include <cstring>
#include <string>
#include <conio.h>

#define FILENAME "C:/Users/Boss/Desktop/BankProject/database.txt"
#define SIZEBYTES 17
using std::cout;
using std::endl;


struct Client {
	double balance;
	unsigned int pin;
	char card_num[17];
};


BOOL WriteToDatabase() {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle" << endl;
		return FALSE;
	}
	SetFilePointer(hdatabase, 0, NULL, FILE_END);
	Client k;
	cout << "Enter card number: " << endl;
	std::cin >> k.card_num;
	if (strlen(k.card_num) != 16) {
		cout << "Invalid card number " << endl;
		return FALSE;
	}
	k.balance = 100.5;
	k.pin = (atoi(&k.card_num[12]) * 100) + (atoi(&k.card_num[13]) * 10) + atoi(&k.card_num[14]);
	cout << "PIN: " << k.pin << endl;
	BOOL writedata;
	DWORD wrotebytes;
	writedata = WriteFile(hdatabase, &k, sizeof(Client), &wrotebytes, NULL);
	if (wrotebytes < sizeof(Client)) {
		cout << "Wrote to data less than requird " << GetLastError() << endl;
	}
	CloseHandle(hdatabase);
	return writedata;
}
BOOL WriteToDatabase(const Client& k) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(hdatabase, 0, NULL, FILE_END);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle" << endl;
		return FALSE;
	}
	BOOL writedata;
	DWORD wrotebytes;
	writedata = WriteFile(hdatabase, &k, sizeof(Client), &wrotebytes, NULL);
	if (wrotebytes < sizeof(Client)) {
		cout << "Wrote to data less than requird " << GetLastError() << endl;
	}
	CloseHandle(hdatabase);
	return writedata;
}
BOOL ReadFromDatabase(Client& k) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle" << endl;
		return FALSE;
	}
	BOOL readdata;
	DWORD readbytes;
	readdata = ReadFile(hdatabase, &k, sizeof(Client), &readbytes, NULL);
	if (readbytes < sizeof(Client)) {
		cout << "Read less than requird " << GetLastError() << endl;
	}
	cout << "Read from database card: " << k.card_num << "with balance" << k.balance << endl;
	return readdata;
}
//чтение из базыданных
void WriteTo(HANDLE hPipe) {
	BOOL write_pipe;
	DWORD written_Bytes = 0;
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_ALL, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	char card_num[SIZEBYTES] = { 0 };
	Client temp;
	ReadFromDatabase(temp);
	write_pipe = WriteFile(hPipe, &temp, sizeof(Client), &written_Bytes, NULL);
	if (written_Bytes != sizeof(Client)) {
		std::cout << "Written less bytes " << written_Bytes << std::endl;
		CloseHandle(hdatabase);
		exit(1);
	}
	CloseHandle(hdatabase);
}
//Запись в базуданных
void Readfrom(HANDLE hPipe) {
	BOOL read_pipe;
	DWORD read_Bytes = 0;
	Client k1;
	Client k2;
	cout << "Reading form pipe..." << endl;
	read_pipe = ReadFile(hPipe, &k1, sizeof(Client), &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes != sizeof(Client)) {
			std::cout << "Read less bytes " << read_Bytes << " " << GetLastError() << std::endl;
			exit(1);
		}
		while (ReadFromDatabase(k2)) {
			if (strcmp(k1.card_num, k2.card_num) != 0) {
				WriteToDatabase(k1);
				cout << "New number of card: " << k1.card_num << endl;
				break;
			}
			else {
				cout << "Such card number already exist " << endl;
			}
		}
	}
}


void WriteThread(HANDLE hPipe, HANDLE hSemWr) {
	DWORD id;
	HANDLE hThread = CreateThread(
		NULL,
		0,
		[](LPVOID param) -> DWORD {
			HANDLE hPipe = (HANDLE)param;
			WriteTo(hPipe);
			return 0;
		}, hPipe, 0, &id);
	if (hThread == NULL) { std::cout << "Error with thread " << GetLastError() << std::endl; }

	std::cout << "Thread with id " << id << " start writing" << std::endl;

	if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
	std::cout << "Thread with id " << id << " end" << std::endl;
	if (hThread != NULL) CloseHandle(hThread);

	if (hSemWr != NULL) { ReleaseSemaphore(hSemWr, 1, NULL); }
}
void ReadThread(HANDLE hPipe) {
	DWORD id;
	HANDLE hThread = CreateThread(
		NULL,
		0,
		[](LPVOID param) -> DWORD {
			HANDLE hPipe = (HANDLE)param;
			Readfrom(hPipe);
			return 0;
		}, hPipe, 0, &id);
	std::cout << "Thread with id " << id << " start reading" << std::endl;
	if (hThread != NULL) { WaitForSingleObject(hThread, INFINITE); }
	std::cout << "Thread with id " << id << " end" << std::endl;
	if (hThread != NULL) CloseHandle(hThread);
}

void Initialize(HANDLE& hSemSigToWrite, HANDLE& hSemWr, HANDLE& hSemRd, HANDLE& hPipe, const DWORD& p_output, const DWORD& p_input, BOOL& connect_pipe) {
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
		exit(0);
	}
	else {
		std::cout << "========= Server start =========" << std::endl;
		connect_pipe = ConnectNamedPipe(hPipe, NULL);
	}
}
DWORD ImitatioOfWork(LPVOID) {
	Sleep(500);
	std::cout << "Working..." << std::endl;
	return 0;
}

void Admin(HANDLE hImitatioOfWork) {
	char code;
	code = _getch();
	if (code == '3')
	{
		if (hImitatioOfWork != NULL) SuspendThread(hImitatioOfWork);
		WriteToDatabase();
		if (hImitatioOfWork != NULL) ResumeThread(hImitatioOfWork);
	}
}
HANDLE hPipe = NULL;
HANDLE hSemWr = NULL;
HANDLE hSemRd = NULL;
HANDLE hSemEx = NULL;

int main()
{
	int i = 0;
	BOOL connect_pipe;
	BOOL FlushFile;
	BOOL disconect_pipe;
	BOOL closehandle;
	BOOL CloseHandleThreadWrite;
	HANDLE hSemSigToWrite = NULL;
	DWORD p_input = sizeof(Client);
	DWORD p_output = sizeof(Client);
	HANDLE hThreadReading = NULL;
	HANDLE hThreadWriting = NULL;
	HANDLE hThreadExit = NULL;
	HANDLE hAddtodatabase = NULL;
	HANDLE hImitatioOfWork = NULL;
	HANDLE hAdmin = NULL;
	//initialize
	hSemSigToWrite = CreateSemaphoreA(NULL, 0, 1, "SemSigToWrite");
	hSemWr = CreateSemaphoreA(NULL, 0, 1, "MySemWr");
	hSemRd = CreateSemaphoreA(NULL, 0, 1, "MySemRd");
	hSemEx = CreateSemaphoreA(NULL, 0, 1, "MySemEx");
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
	connect_pipe = ConnectNamedPipe(hPipe, NULL);
	char code;
	if (connect_pipe) {
		while (1) {
			if (hImitatioOfWork == NULL) hImitatioOfWork = CreateThread(NULL, 0, ImitatioOfWork, NULL, 0, NULL);
			Admin(hImitatioOfWork);
			if (hThreadWriting == NULL) {
				hThreadWriting = CreateThread(NULL,
					0,
					[](LPVOID param) -> DWORD {
						HANDLE hSemSigToWrite = (HANDLE)param;
						WaitForSingleObject(hSemSigToWrite, INFINITE);

						WriteThread(hPipe, hSemWr);
						if (hSemSigToWrite != NULL) CloseHandle(hSemSigToWrite);
						return 0;
					},
					hSemSigToWrite, 0, NULL);
			}

			//signal to read
			if (hThreadReading == NULL) {
				hThreadReading = CreateThread(NULL,
					0,
					[](LPVOID param) -> DWORD {
						HANDLE hSemRd = (HANDLE)param;
						if (hSemRd != NULL) WaitForSingleObject(hSemRd, INFINITE);
						ReadThread(hPipe);
						return 0;
					}, hSemRd, 0, NULL);

				hThreadExit = CreateThread(NULL,
					0,
					[](LPVOID param) -> DWORD {
						HANDLE hSemEx = (HANDLE)param;
						if (hSemEx != NULL) WaitForSingleObject(hSemEx, INFINITE);
						exit(1);
						return 0;
					}, hSemEx, 0, NULL);
			}

			if (hAddtodatabase == NULL) {
				hAddtodatabase = CreateThread(NULL, 0,
					[](LPVOID)-> DWORD {
						char code;
						std::cin >> code;
						if (code == '3') { WriteToDatabase(); }
						return 0;
					}, NULL, 0, NULL);
			}

		}
	}
	else {
		std::cout << "Error with connecting to PIPE " << GetLastError() << std::endl;
	}
	if (hImitatioOfWork != NULL) WaitForSingleObject(hImitatioOfWork, INFINITE);
	if (hAddtodatabase != NULL) WaitForSingleObject(hAddtodatabase, INFINITE);
	if (hThreadWriting != NULL) WaitForSingleObject(hThreadWriting, INFINITE);
	if (hThreadWriting != NULL) CloseHandleThreadWrite = CloseHandle(hThreadWriting);
	FlushFile = FlushFileBuffers(hPipe);
	disconect_pipe = DisconnectNamedPipe(hPipe);
	return 0;
}
