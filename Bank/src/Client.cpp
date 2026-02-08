#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <mutex>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define SIZEBYTES 17
BOOL GlobalBwrite = FALSE;


struct Client {
	double balance;
	int pin;
	char card_num[17];
};


void ReadFromFile(HANDLE hPipe) {
	BOOL read_pipe;
	string temp_card;
	DWORD read_Bytes = 0;
	Client k;
	read_pipe = ReadFile(hPipe, &k, sizeof(Client), &read_Bytes, NULL);

	if (read_pipe) {
		if (read_Bytes < sizeof(Client)) {
			cout << "Read less " << GetLastError() << endl;
			exit(1);
		}
		else {
			cout << "Enter your card number " << endl;
			cin >> temp_card;
			if (temp_card.size() != 16) { cout << "Invalid card number " << endl; }
			else
			{
				if (strcmp(k.card_num, temp_card.c_str()) == 0)
				{
					cout << "Access approved" << endl;
					cout << "Number of card: " << k.card_num << endl;
				}
				else cout << "Access denied " << endl;
			}
		}
	}
	else {
		cout << "Error with reading " << GetLastError() << endl;
		exit(1);
	}
}
BOOL WritetoFile(HANDLE hPipe) {
	BOOL write_pipe = FALSE;
	DWORD wrote_Bytes = 0;
	Client k;
	cout << "Enter your card number " << endl;
	cin >> k.card_num;
	k.balance = 100.5;
	k.pin = 1;
	if (strlen(k.card_num) != 16) { cout << "Invalid card number " << endl; return FALSE; }
	else write_pipe = WriteFile(hPipe, &k, sizeof(Client), &wrote_Bytes, NULL);
	if (write_pipe) {
		if (wrote_Bytes < 29) {
			cout << "Wrote less " << GetLastError() << endl;
			exit(1);
		}
		else {
			cout << "Number of card: " << k.card_num << endl;
			return TRUE;
		}
	}
	else {
		cout << "Error with writting " << GetLastError() << endl;
		exit(1);
	}
}
void ThreadRead(LPVOID param, HANDLE hSemWr, HANDLE hPipe, HANDLE hSemSigToWrite) {
	ReleaseSemaphore(hSemSigToWrite, 1, NULL);
	if (hSemWr != NULL) WaitForSingleObject(hSemWr, INFINITE);
	DWORD id1 = 0;
	HANDLE hThread = CreateThread(NULL, 0,
		[](LPVOID lpParam) -> DWORD {
			HANDLE hPipe = (HANDLE)lpParam;
			ReadFromFile(hPipe);
			return 0;
		}
	, hPipe, 0, &id1);
	cout << "Thread with id " << id1 << " start" << endl;
	if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
	cout << "Thread with id " << id1 << " end" << endl;
	if (hThread != NULL) CloseHandle(hThread);
}
void ThreadWrite(LPVOID param, HANDLE hPipe, HANDLE hSemRd) {
	long var = 0;
	DWORD id2 = 0;
	HANDLE hThread1 = CreateThread(NULL, 0,
		[](LPVOID lpParam) -> DWORD {
			HANDLE hPipe = (HANDLE)lpParam;
			GlobalBwrite = WritetoFile(hPipe);
			return 0;
		}
	, hPipe, 0, &id2);
	cout << "Thread with id " << id2 << " start" << endl;
	if (hThread1 != NULL) WaitForSingleObject(hThread1, INFINITE);
	cout << "Thread with id " << id2 << " end" << endl;
	if (hThread1 != NULL) CloseHandle(hThread1);
	if (GlobalBwrite) if (hSemRd != NULL) ReleaseSemaphore(hSemRd, 1, NULL);
}

void Menu() {
	cout << "======== MENU =======" << endl;
	cout << "1. Add card to Bank base" << endl;
	cout << "2. Operation with card" << endl;
}

int main()
{
	HANDLE hPipe;
	HANDLE hSemRd;
	HANDLE hSemSigToWrite;
	HANDLE hSemSigToExite;
	hSemRd = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, "MySemRd");
	HANDLE hSemWr;
	hSemWr = OpenSemaphoreA(SYNCHRONIZE, FALSE, "MySemWr");
	hSemSigToWrite = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, "SemSigToWrite");
	hSemSigToExite = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE, FALSE, "MySemEx");
	hPipe = CreateFileA(
		"\\\\.\\pipe\\Server_pipe"
		, GENERIC_ALL
		, FILE_SHARE_WRITE | FILE_SHARE_READ
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	if (hPipe == INVALID_HANDLE_VALUE) {
		cout << "Error with connecting to pipe " << GetLastError() << endl;
		exit(1);
	}
	else {
		cout << "========= Client start =========" << endl;
	}

	DWORD p_input = 20;
	DWORD p_output = 20;
	char choose;
	int i = 0;
	while (1) {
		Menu();
		cout << "Waiting..." << endl;

		cin >> choose;
		switch (choose) {
		case '2':
			if (hSemSigToWrite) ThreadRead(hPipe, hSemWr, hPipe, hSemSigToWrite);
			break;

			//Запись в пайп 
		case '1': ThreadWrite(hPipe, hPipe, hSemRd);
			break;


		case '3':if (hSemSigToExite != NULL)
			ReleaseSemaphore(hSemSigToExite, 1, NULL);
			return 0;
			break;

		}
	}
	CloseHandle(hPipe);
	return 0;
}
