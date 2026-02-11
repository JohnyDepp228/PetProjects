#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <cstring>
#include <string>
#include <conio.h>

#define FILENAME "C:/Users/Boss/Desktop/BankProject/database.txt"
#define SIZEBYTES 17
using std::cout;
using std::endl;

HANDLE hPipe = NULL;
HANDLE hEventWr = NULL;
HANDLE hEventRd = NULL;
HANDLE hEventEx = NULL;
HANDLE hAccess = NULL;
HANDLE hDenied = NULL;

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
	Client k = { 0.0,000,"0000000000000000" };
	cout << "Enter card number: " << endl;
	std::cin >> k.card_num;
	if (strlen(k.card_num) != 16) {
		cout << "Invalid card number " << endl;
		return FALSE;
	}
	k.balance = 100.5;
	k.pin = atoi(&k.card_num[13]);
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
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(hdatabase, 0, NULL, FILE_END);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle Write" << endl;
		if (hdatabase != NULL) CloseHandle(hdatabase);
		return FALSE;
	}
	BOOL writedata;
	DWORD wrotebytes;
	writedata = WriteFile(hdatabase, &k, sizeof(Client), &wrotebytes, NULL);
	if (wrotebytes < sizeof(Client)) {
		cout << "Wrote to data less than requird " << GetLastError() << endl;
	}
	if (hdatabase != NULL) CloseHandle(hdatabase);
	return writedata;
}
BOOL ReadFromDatabase(Client& k) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle Read" << GetLastError() << endl;
		if (hdatabase != NULL) CloseHandle(hdatabase);
		return FALSE;
	}
	BOOL readdata;
	DWORD readbytes;
	readdata = ReadFile(hdatabase, &k, sizeof(Client), &readbytes, NULL);
	if (readbytes < sizeof(Client)) {
		cout << "Read less than requird " << GetLastError() << endl;
	}
	cout << "Read from database card: " << k.card_num << "with balance" << k.balance << endl;
	if (hdatabase != NULL) CloseHandle(hdatabase);
	return readdata;
}
//чтение из базыданных
void WriteTo(HANDLE hPipe) {
	BOOL write_pipe;
	DWORD written_Bytes = 0;
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
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
//Проверка доступа + чтение
void Readfrom(HANDLE hPipe) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle Read" << GetLastError() << endl;
		if (hdatabase != NULL) CloseHandle(hdatabase);
		return;
	}
	BOOL read_pipe;
	DWORD read_Bytes = 0;
	DWORD readbytes;
	bool bAccess = FALSE;
	Client k1 = { 0.0,000,"0000000000000000" };
	Client k2 = { 0.0,000,"0000000000000000" };
	BOOL eventAccsig = FALSE;
	BOOL eventDensig = FALSE;
	cout << "Reading form pipe..." << endl;
	read_pipe = ReadFile(hPipe, &k1, sizeof(Client), &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes != sizeof(Client)) {
			std::cout << "Read less bytes " << read_Bytes << " " << GetLastError() << std::endl;
			exit(1);
		}
		SetFilePointer(hdatabase, 0, NULL, FILE_BEGIN);
		while (ReadFile(hdatabase, &k2, sizeof(Client), &readbytes, NULL)) {
			if (readbytes == 0) {
				cout << "Reached end of file and no data find " << endl;
				eventDensig = SetEvent(hDenied);
				if (eventDensig == FALSE) {
					cout << "Didn't sent signl to deny " << endl;
				}
				else {
					cout << "Sent signal to deny successfully" << endl;
				}
				break;
			}
			if (readbytes < sizeof(Client)) {
				cout << "Read less than requird " << GetLastError() << endl;
				return;
			}
			if (strcmp(k1.card_num, k2.card_num) == 0 && k1.pin == k2.pin) {
				eventAccsig = SetEvent(hAccess);
				if (eventAccsig == FALSE) {
					cout << "Didn't sent signl to access " << endl;
				}
				else {
					cout << "Sent signal to access successfully" << endl;
				}
				bAccess = TRUE;
				break;
			}
		}
	}
	if (!bAccess) cout << "Trying to access" << endl;
	if (hdatabase != NULL) CloseHandle(hdatabase);
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

	if (hEventWr != NULL) { SetEvent(hEventWr); }
}

DWORD ImitatioOfWork(LPVOID) {
	while (1) {
		Sleep(500);
		std::cout << "Working..." << std::endl;
	}
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

int main()
{
	BOOL connect_pipe;
	BOOL FlushFile;
	BOOL disconect_pipe;
	BOOL CloseHandleThreadWrite;
	DWORD p_input = sizeof(Client);
	DWORD p_output = sizeof(Client);
	HANDLE hThreadReading = NULL;
	HANDLE hThreadWriting = NULL;
	HANDLE hThreadExit = NULL;
	HANDLE hAddtodatabase = NULL;
	HANDLE hImitatioOfWork = NULL;
	HANDLE hAdmin = NULL;


	//initialize

	hEventWr = CreateEvent(NULL, FALSE, FALSE, L"EventWr");
	hEventRd = CreateEvent(NULL, FALSE, FALSE, L"EventRd");
	hEventEx = CreateEvent(NULL, FALSE, FALSE, L"hEventEx");
	hAccess = CreateEvent(NULL, FALSE, FALSE, L"hAccess");
	hDenied = CreateEvent(NULL, FALSE, FALSE, L"hDenied");
	hPipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\Server_pipe"
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
	if (connect_pipe) {
		while (1) {
			//if (hImitatioOfWork == NULL) hImitatioOfWork = CreateThread(NULL, 0, ImitatioOfWork, NULL, 0, NULL);
			//Admin(hImitatioOfWork);
			if (hThreadReading == NULL) {//тут ошибка с логикой 
				hThreadReading = CreateThread(NULL,
					0,
					[](LPVOID param) -> DWORD {
						HANDLE hEventRd = (HANDLE)param;
						DWORD res = NULL;
						if (hEventRd != NULL) res = WaitForSingleObject(hEventRd, INFINITE);
						if (res == WAIT_OBJECT_0) {
							cout << "Get signal to read from client " << endl;
							Readfrom(hPipe);
						}
						return 0;
					}, hEventRd, 0, NULL);
			}

			if (hThreadExit == NULL) {
				hThreadExit = CreateThread(NULL,
					0,
					[](LPVOID param) -> DWORD {
						HANDLE hEventEx = (HANDLE)param;
						DWORD res = NULL;
						if (hEventEx != NULL) res = WaitForSingleObject(hEventEx, INFINITE);
						if (res == WAIT_OBJECT_0) {
							cout << "Get signal to exit from client " << endl;
						}
						else {
							cout << "No sig get form client to exit " << endl;
						}
						exit(1);
						return 0;
					}, hEventEx, 0, NULL);
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
	if (!FlushFile) cout << "Problem with flushing " << GetLastError() << endl;
	disconect_pipe = DisconnectNamedPipe(hPipe);
	if (hThreadReading != NULL) {
		WaitForSingleObject(hThreadReading, INFINITE);
		CloseHandle(hThreadReading);
	}


	if (hEventWr != NULL) CloseHandle(hEventWr);
	if (hEventRd != NULL) CloseHandle(hEventRd);
	if (hEventEx != NULL) CloseHandle(hEventEx);
	if (hAccess != NULL) CloseHandle(hAccess);
	if (hDenied != NULL) CloseHandle(hDenied);
	return 0;
}
