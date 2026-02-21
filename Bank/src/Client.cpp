#include <iostream>
#include <windows.h>
#include <thread>
#include <chrono>
#include <functional> 
#include <string>
#include <conio.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define SIZEBYTES 17
bool GlobalBwrite = FALSE;

struct Client {
	unsigned int pos;
	double balance;
	unsigned int pin;
	char card_num[17];
};

struct Handles {
	HANDLE hPipe = NULL;
	HANDLE hEventRd = NULL;
	HANDLE hEventWr = NULL;
	HANDLE hEventEx = NULL;
	HANDLE hAccess = NULL;
	HANDLE hDenied = NULL;
	HANDLE hEventAfterOperations = NULL;
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
void ThreadRead(LPVOID param, HANDLE hEventWr, HANDLE hPipe) {
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


void Initialize(Handles& h) {
	h.hEventRd = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EventRd");
	h.hEventWr = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventWr");
	h.hEventEx = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventEx");
	h.hAccess = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hAccess");
	h.hDenied = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hDenied");
	h.hEventAfterOperations = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventAfterOperations");

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

void Operations(Client& k, const Handles& h) {
	char ch;
	int money_amount;
	DWORD wroteBytes = 0;
	bool EventAfterOperationssig = false;
	while (1) {
		cout << "Choose operation: " << endl;
		cout << "1. Withdraw money from the card " << endl;
		cout << "2. Put money into the card " << endl;
		cout << "3. Find out the balance on the card " << endl;
		cout << "4. Exit " << endl;
		ch = _getch();
		switch (ch) {
		case '1':
			cout << "Enter amout of money you want to withdraw: ";
			cin >> money_amount;
			cout << endl;
			if (money_amount > k.balance) {
				cout << "Not enough money on card " << endl;
			}
			else {
				k.balance -= money_amount;
				cout << "Success" << endl;
				cout << "New balance: " << k.balance << endl;
			}
			WriteFile(h.hPipe, &k, sizeof(Client), &wroteBytes, NULL);
			if (wroteBytes != sizeof(Client)) {
				cout << "Error with writing after operations " << GetLastError() << endl;
				return;
			}
			else {
				cout << "Wrote struct succesfully after operations " << endl;
			}
			EventAfterOperationssig = SetEvent(h.hEventAfterOperations);
			if (EventAfterOperationssig == FALSE) {
				cout << "Didn't sent signl to AfterOperationssig " << GetLastError() << endl;
			}
			else {
				cout << "Sent signal to AfterOperationssig successfully" << endl;
			}
			break;
		case '2':
			cout << "Enter amout of money you want to put: ";
			cin >> money_amount;
			cout << endl;
			k.balance += money_amount;
			cout << "Success" << endl;
			cout << "New balance: " << k.balance << endl;
			WriteFile(h.hPipe, &k, sizeof(Client), &wroteBytes, NULL);
			if (wroteBytes != sizeof(Client)) {
				cout << "Error with writing after operations " << GetLastError() << endl;
				return;
			}
			else {
				cout << "Wrote struct succesfully after operations " << endl;
			}
			EventAfterOperationssig = SetEvent(h.hEventAfterOperations);
			if (EventAfterOperationssig == FALSE) {
				cout << "Didn't sent signl to AfterOperationssig " << GetLastError() << endl;
			}
			else {
				cout << "Sent signal to AfterOperationssig successfully" << endl;
			}
			break;
		case '3':
			cout << "Balance on card: " << k.balance << endl; break;
		case '4':
			return;	break;
		}
	}
}

int main()
{
	Handles h;
	Initialize(h);
	char choose;
	bool access = false;
	DWORD approved = NULL;
	DWORD denied = NULL;
	Client k = { 0,0.0,000,"0000000000000000" };
	DWORD readBytes = 0;
	bool readfile = false;
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
				readfile = ReadFile(h.hPipe, &k, sizeof(Client), &readBytes, NULL);
				if (readfile) {
					if (readBytes != sizeof(Client)) {
						cout << "Error with reading after access " << GetLastError() << endl;
						return 12;
					}
					else {
						cout << "Read struct succesfully after access " << endl;
						access = true;
					}
				}
				else {
					cout << "Can't read file " << GetLastError() << endl;
				}
			}
			if (h.hDenied != NULL) denied = WaitForSingleObject(h.hDenied, 2000);
			if (denied == WAIT_OBJECT_0) {
				cout << "Denied" << endl;
			}
			if (access) {
				Operations(k, h);

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
