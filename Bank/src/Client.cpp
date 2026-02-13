#include "Libs.h"

bool GlobalBwrite = FALSE;

struct Client {
	double balance;
	int pin;
	char card_num[17];
};

struct Handles {
	HANDLE hPipe = NULL;
	HANDLE hEventRd = NULL;
	HANDLE hEventWr = NULL;
	HANDLE hEventEx = NULL;
	HANDLE hAccess = NULL;
	HANDLE hDenied = NULL;
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
bool WritetoFile(HANDLE hPipe) {
	bool write_pipe = false;
	DWORD wrote_Bytes = 0;
	Client k;
	cout << "Enter your card number " << endl;
	cin >> k.card_num;
	if (strlen(k.card_num) != 16) { cout << "Invalid card number " << endl; return false; }
	cout << "Enter pincode " << endl;
	cin >> k.pin;
	if (k.pin < 100) { cout << "Invalid pin number " << endl; return false; }
	write_pipe = WriteFile(hPipe, &k, sizeof(Client), &wrote_Bytes, NULL);
	if (write_pipe) {
		if (wrote_Bytes < sizeof(Client)) {
			cout << "Wrote less " << GetLastError() << endl;
			return false;
		}
		else {
			cout << "Successfully wrote data to pipe " << wrote_Bytes << endl;
			return true;
		}
	}
	else {
		cout << "Error with writting " << GetLastError() << endl;
		return false;
	}
}
void ThreadRead(LPVOID param, HANDLE hEventWr, HANDLE hPipe, HANDLE hSemSigToWrite) {
	ReleaseSemaphore(hSemSigToWrite, 1, NULL);
	if (hEventWr != NULL) WaitForSingleObject(hEventWr, INFINITE);
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
void ThreadWrite(LPVOID param, HANDLE hPipe, HANDLE hEventRd) {
	DWORD id2 = 0;
	BOOL Eventreadsig = FALSE;
	GlobalBwrite = FALSE;
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
	if (GlobalBwrite) {
		if (hEventRd != NULL) {
			Eventreadsig = SetEvent(hEventRd);
			if (Eventreadsig == FALSE) {
				cout << "Didn't sent signl to read " << GetLastError() << endl;
			}
			else {
				cout << "Sent signal to read successfully" << endl;
			}
		}
	}
}

void Menu() {
	cout << "======== MENU =======" << endl;
	cout << "1. Operation with card" << endl;
}


bool Initialize(Handles& h) {
	h.hEventRd = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EventRd");
	h.hEventWr = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventWr");
	h.hEventEx = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventEx");
	h.hAccess = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hAccess");
	h.hDenied = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hDenied");

	h.hPipe = CreateFileW(
		L"\\\\.\\pipe\\Server_pipe"
		, GENERIC_ALL
		, FILE_SHARE_WRITE | FILE_SHARE_READ
		, NULL
		, OPEN_EXISTING
		, FILE_ATTRIBUTE_NORMAL
		, NULL);
	if (h.hPipe == INVALID_HANDLE_VALUE) {
		cout << "Error with connecting to pipe " << GetLastError() << endl;
		exit(1);
	}
	else {
		cout << "========= Client start =========" << endl;
	}
}

void CloseHandles(Handles& h) {
	CloseHandle(h.hPipe);
	CloseHandle(h.hEventRd);
	CloseHandle(h.hEventWr);
	CloseHandle(h.hAccess);
	CloseHandle(h.hDenied);
	CloseHandle(h.hEventEx);
}


int main()
{
	Handles h;
	Initialize(h);


	char choose;
	DWORD approved = NULL;
	DWORD denied = NULL;
	while (1) {
		Menu();
		cout << "Waiting..." << endl;

		cin >> choose;
		switch (choose) {

		case '1':
			ThreadWrite(h.hPipe, h.hPipe, h.hEventRd);
			if (h.hAccess != NULL) approved = WaitForSingleObject(h.hAccess, 2000);
			if (approved == WAIT_OBJECT_0) {
				cout << "Approved " << endl;
			}
			if (h.hDenied != NULL) denied = WaitForSingleObject(h.hDenied, 2000);
			if (denied == WAIT_OBJECT_0) {
				cout << "Denied" << endl;
			}
			break;

		case '3':
			if (h.hEventEx != NULL)
			{
				SetEvent(h.hEventEx);
			}
			cout << "Sent signal to exit " << endl;
			return 0;
			break;

		}
	}


	CloseHandles(h);
	return 0;
}
