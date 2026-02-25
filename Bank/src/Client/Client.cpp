#include "Client.h"

bool GlobalBwrite = FALSE;

void ClientClass::Initialize() {
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
	h.hEventRd = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EventRd");
	h.hEventWr = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventWr");
	if (h.hEventRd == NULL) {
		cout << "Can't open hEventRd " << GetLastError() << endl;
		return;
	}
	h.hEventEx = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventEx");
	if (h.hEventEx == NULL) {
		cout << "Can't open hEventEx " << GetLastError() << endl;
		return;
	}
	h.hAccess = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hAccess");
	if (h.hAccess == NULL) {
		cout << "Can't open hAccess " << GetLastError() << endl;
		return;
	}
	h.hDenied = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hDenied");
	if (h.hDenied == NULL) {
		cout << "Can't open hDenied " << GetLastError() << endl;
		return;
	}
	h.hEventExRd = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventExRd");
	if (h.hEventExRd == NULL) {
		cout << "Can't open hEventExRd " << GetLastError() << endl;
		return;
	}
	h.hEventExWr = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventExWr");
	if (h.hEventExWr == NULL) {
		cout << "Can't open hEventExWr " << GetLastError() << endl;
		return;
	}
	h.hEventAfterOperations = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"hEventAfterOperations");
	if (h.hEventAfterOperations == NULL) {
		cout << "Can't open hEventAfterOperations " << GetLastError() << endl;
		return;
	}
}
void ClientClass::Menu() {
	DWORD approved = NULL;
	DWORD denied = NULL;
	char choose;
	DWORD readBytes = 0;
	bool readfile = false;
	bool access = false;
	Client k1 = { 0,0.0,000,"0000000000000000" };
	while (1) {
		cout << "======== MENU =======" << endl;
		cout << "1. Operation with card" << endl;
		cout << "3. End session" << endl;
		choose = _getch();
		switch (choose) {

		case '1':
			ThreadWrite();
			if (h.hAccess != NULL) approved = WaitForSingleObject(h.hAccess, 2000);
			if (approved == WAIT_OBJECT_0) {
				cout << "Approved " << endl;
				readfile = ReadFile(h.hPipe, &k1, sizeof(Client), &readBytes, NULL);
				if (readfile) {
					if (readBytes != sizeof(Client)) {
						cout << "Error with reading after access " << GetLastError() << endl;
						return;
					}
					else {
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
				access = false;
			}
			if (access) {
				Operations(k1);
			}
			break;

		case '3':
			if (h.hEventEx != NULL)
			{
				SetEvent(h.hEventEx);
				SetEvent(h.hEventExRd);
				SetEvent(h.hEventExWr);
			}
			return;
			break;
		}
	}
}

bool ClientClass::WritetoPipe() {
	bool write_pipe = false;
	DWORD wrote_Bytes = 0;
	Client k;
	cout << "Enter your card number " << endl;
	cin >> k.card_num;
	if (strlen(k.card_num) != 16) { cout << "Invalid card number " << endl; return false; }
	cout << "Enter pincode " << endl;
	cin >> k.pin;
	if (k.pin < 100) { cout << "Invalid pin number " << endl; return false; }
	write_pipe = WriteFile(h.hPipe, &k, sizeof(Client), &wrote_Bytes, NULL);
	if (write_pipe) {
		if (wrote_Bytes < sizeof(Client)) {
			cout << "Wrote less " << GetLastError() << endl;
			return false;
		}
		else {
			return true;
		}
	}
	else {
		cout << "Error with writting " << GetLastError() << endl;
		return false;
	}
}
void ClientClass::ThreadWrite() {
	DWORD id2 = 0;
	BOOL Eventreadsig = FALSE;
	GlobalBwrite = FALSE;
	HANDLE hThread1 = CreateThread(NULL, 0,
		[](LPVOID lpParam) -> DWORD {
			ClientClass* classPtr = (ClientClass*)lpParam;
			GlobalBwrite = classPtr->WritetoPipe();
			return 0;
		}
	, this, 0, &id2);
	if (hThread1 != NULL) WaitForSingleObject(hThread1, INFINITE);
	if (hThread1 != NULL) CloseHandle(hThread1);
	if (GlobalBwrite) {
		if (h.hEventRd != NULL) {
			Eventreadsig = SetEvent(h.hEventRd);
			if (Eventreadsig == FALSE) {
				cout << "Didn't sent signl to read " << GetLastError() << endl;
			}
		}
	}
}

void ClientClass::Operations(Client& k) {
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
			EventAfterOperationssig = SetEvent(h.hEventAfterOperations);
			if (EventAfterOperationssig == FALSE) {
				cout << "Didn't sent signl to AfterOperationssig " << GetLastError() << endl;
			}
			break;
		case '3':
			cout << "Balance on card: " << k.balance << endl; break;
		case '4':
			return;	break;
		}
	}
}


void ClientClass::CloseHandles() {
	cout << "Closing Handles " << endl;
	if (h.hPipe != NULL) CloseHandle(h.hPipe);
	if (h.hEventRd != NULL) CloseHandle(h.hEventRd);
	if (h.hEventWr != NULL) CloseHandle(h.hEventWr);
	if (h.hAccess != NULL) CloseHandle(h.hAccess);
	if (h.hDenied != NULL) CloseHandle(h.hDenied);
	if (h.hEventRd != NULL) CloseHandle(h.hEventRd);
	if (h.hEventWr != NULL) CloseHandle(h.hEventWr);
	if (h.hEventExRd != NULL) CloseHandle(h.hEventExRd);
	if (h.hEventExWr != NULL) CloseHandle(h.hEventExWr);
	cout << "Closed Handles " << endl;
}
