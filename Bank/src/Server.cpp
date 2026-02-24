#include "Server.h"

void Server::incrypt(char* card_num, int size) {
	for (int i = 0; i < size; i++) {
		card_num[i] ^= keyToencrypt;
	}
}

void Server::decrypt(char* card_num, int size) {
	for (int i = 0; i < size; i++) {
		card_num[i] ^= keyToencrypt;
	}
}

bool Server::WriteToDatabaseCard() {
	LARGE_INTEGER sizeOfFile;
	bool readfile = false;
	DWORD readbytes = 0;
	Client temp = { 0,0.0,000,"0000000000000000" };
	Client k = { 0,0.0,000,"0000000000000000" };
	BOOL writedata;
	DWORD wrotebytes;
	int BytesToMove = sizeof(Client);
	DWORD pos = 0;
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_ALL, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle" << endl;
		return FALSE;
	}
	cout << "Enter card number: " << endl;
	std::cin >> k.card_num;
	if (strlen(k.card_num) != 16) {
		cout << "Invalid card number " << endl;
		return FALSE;
	}
	k.balance = 100.5;
	k.pin = atoi(&k.card_num[13]);
	cout << "PIN: " << k.pin << endl;
	incrypt(k.card_num, 16);
	GetFileSizeEx(hdatabase, &sizeOfFile);
	if (sizeOfFile.QuadPart >= sizeof(Client)) {
		SetLastError(0);
		pos = SetFilePointer(hdatabase, -BytesToMove, NULL, FILE_END);
		readfile = ReadFile(hdatabase, &temp, sizeof(Client), &readbytes, NULL);
		if (readfile) {
			if (readbytes != sizeof(Client)) {
				cout << "Error with reading from database for index " << GetLastError() << endl;
				exit(0);
			}
		}
		else {
			cout << "Can't read file " << GetLastError() << endl;
		}
		k.pos = temp.pos + 1;
	}
	else {
		SetFilePointer(hdatabase, 0, NULL, FILE_BEGIN);
		k.pos = 0;
	}
	SetFilePointer(hdatabase, 0, NULL, FILE_END);
	writedata = WriteFile(hdatabase, &k, sizeof(Client), &wrotebytes, NULL);
	if (wrotebytes < sizeof(Client)) {
		cout << "Wrote to data less than requird " << GetLastError() << endl;
	}
	CloseHandle(hdatabase);
	return writedata;
}

bool Server::WriteToDatabase(Client& k) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned int pos = k.pos * sizeof(Client);
	cout << "Index to write: " << k.pos << endl;
	BOOL writedata;
	DWORD wrotebytes;
	incrypt(k.card_num, 16);
	SetFilePointer(hdatabase, pos, NULL, FILE_BEGIN);
	writedata = WriteFile(hdatabase, &k, sizeof(Client), &wrotebytes, NULL);
	if (wrotebytes < sizeof(Client)) {
		cout << "Wrote to data less than requird " << GetLastError() << endl;
	}
	if (hdatabase != NULL) CloseHandle(hdatabase);
	return writedata;
}

bool Server::ReadFromDatabase(Client& k) {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle Read" << GetLastError() << endl;
		if (hdatabase != NULL) CloseHandle(hdatabase);
		return FALSE;
	}
	BOOL readdata;
	DWORD readbytes;
	readdata = ReadFile(hdatabase, &k, sizeof(Client), &readbytes, NULL);
	decrypt(k.card_num, 16);
	if (readbytes < sizeof(Client)) {
		cout << "Read less than requird " << GetLastError() << endl;
	}
	cout << "Read from database card: " << k.card_num << "with balance" << k.balance << endl;
	if (hdatabase != NULL) CloseHandle(hdatabase);
	return readdata;
}
//чтение из базыданных и запись в пайп
void Server::WriteTo() {
	BOOL write_pipe;
	DWORD written_Bytes = 0;
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	char card_num[SIZEBYTES] = { 0 };
	Client temp;
	ReadFromDatabase(temp);
	write_pipe = WriteFile(ev.hPipe, &temp, sizeof(Client), &written_Bytes, NULL);
	if (written_Bytes != sizeof(Client)) {
		std::cout << "Written less bytes " << written_Bytes << std::endl;
		CloseHandle(hdatabase);
		exit(1);
	}
	CloseHandle(hdatabase);
}
//Проверка доступа + чтение
void Server::Readfrom() {
	HANDLE hdatabase = CreateFileA(FILENAME, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hdatabase == INVALID_HANDLE_VALUE || hdatabase == NULL) {
		cout << "Invalid file handle Read" << GetLastError() << endl;
		if (hdatabase != NULL) CloseHandle(hdatabase);
		return;
	}
	DWORD written_Bytes = 0;
	BOOL read_pipe;
	DWORD read_Bytes = 0;
	DWORD readbytes;
	bool bAccess = FALSE;
	Client k1 = { 0,0.0,000,"0000000000000000" };
	Client k2 = { 0,0.0,000,"0000000000000000" };
	BOOL eventAccsig = FALSE;
	BOOL eventDensig = FALSE;
	cout << "Reading form pipe..." << endl;
	read_pipe = ReadFile(ev.hPipe, &k1, sizeof(Client), &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes != sizeof(Client)) {
			std::cout << "Read less bytes " << read_Bytes << " " << GetLastError() << std::endl;
			if (hdatabase != NULL) CloseHandle(hdatabase);
			exit(1);
		}
		SetFilePointer(hdatabase, 0, NULL, FILE_BEGIN);
		while (ReadFile(hdatabase, &k2, sizeof(Client), &readbytes, NULL)) {
			decrypt(k2.card_num, 16);
			if (readbytes == 0) {
				cout << "Reached end of file and no data find " << endl;
				eventDensig = SetEvent(ev.hDenied);
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
				if (hdatabase != NULL) CloseHandle(hdatabase);
				return;
			}
			if (strcmp(k1.card_num, k2.card_num) == 0 && k1.pin == k2.pin) {
				WriteFile(ev.hPipe, &k2, sizeof(Client), &written_Bytes, NULL);
				if (written_Bytes != sizeof(Client)) {
					std::cout << "Written less bytes " << written_Bytes << "Error code: " << GetLastError() << std::endl;
					exit(1);
				}
				eventAccsig = SetEvent(ev.hAccess);
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

void Server::ExitTread() {
	EventHandles* evN = new EventHandles;
	evN->hAccess = ev.hAccess;
	evN->hPipe = ev.hPipe;
	evN->hDenied = ev.hDenied;
	evN->hEventEx = ev.hEventEx;
	evN->hEventRd = ev.hEventRd;
	evN->hEventWr = ev.hEventWr;
	evN->ptrClass = this;
	if (h.hThreadExit == NULL) {
		h.hThreadExit = CreateThread(NULL,
			0,
			[](LPVOID param) -> DWORD {
				EventHandles* ev1 = (EventHandles*)param;
				DWORD res = NULL;
				if (ev1->hEventEx != NULL) res = WaitForSingleObject(ev1->hEventEx, INFINITE);
				if (res == WAIT_OBJECT_0) {
					cout << "Get signal to exit from client " << endl;
					ev1->ptrClass->stopWork = true;
					delete ev1;
					return 1;
				}
				else {
					cout << "No sig get form client to exit " << endl;
				}
				delete ev1;
				return 0;
			}, evN, 0, NULL);
	}
}

void Server::WriteThread() {
	DWORD id;
	EventHandles* evN = new EventHandles;
	evN->hAccess = ev.hAccess;
	evN->hPipe = ev.hPipe;
	evN->hDenied = ev.hDenied;
	evN->hEventEx = ev.hEventEx;
	evN->hEventExWr = ev.hEventExWr;
	evN->hEventRd = ev.hEventRd;
	evN->hEventWr = ev.hEventWr;
	evN->hEventAfterOperations = ev.hEventAfterOperations;
	evN->ptrClass = this;
	h.hThreadWriting = CreateThread(
		NULL,
		0,
		[](LPVOID param) -> DWORD {
			DWORD readbytes = 0;
			bool readfile;
			EventHandles* ev1 = (EventHandles*)param;
			Client temp_k = { 0,0.0,000,"0000000000000000" };
			HANDLE waitHandles[2] = { ev1->hEventAfterOperations, ev1->hEventExWr };
			while (1) {
				DWORD dwEvent = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
				if (waitHandles[0] == NULL) {
					cout << "ev1->hEventWr is NULL! " << GetLastError() << endl;
				}
				else if (waitHandles[1] == NULL) {
					cout << "hEventExWr is NULL! " << GetLastError() << endl;
				}
				if (dwEvent == WAIT_OBJECT_0 + 1) {
					cout << "Stop writing thread " << endl;
					delete ev1;
					return 2;
				}
				else if (dwEvent == WAIT_OBJECT_0) {
					readfile = ReadFile(ev1->hPipe, &temp_k, sizeof(Client), &readbytes, NULL);
					if (readfile) {
						if (readbytes != sizeof(Client)) {
							cout << "Error with reading from pipe from client after operations " << GetLastError() << endl;
							exit(0);
						}
						else {
							ev1->ptrClass->WriteToDatabase(temp_k);
						}
					}
					else {
						cout << "Can't read from pipe from client after operations " << GetLastError() << endl;
					}
				}
			}
			delete ev1;
			return 0;
		}, evN, 0, &id);
}

void Server::ReadThread() {
	EventHandles* evN = new EventHandles;
	evN->hAccess = ev.hAccess;
	evN->hPipe = ev.hPipe;
	evN->hDenied = ev.hDenied;
	evN->hEventEx = ev.hEventEx;
	evN->hEventExRd = ev.hEventExRd;
	evN->hEventRd = ev.hEventRd;
	evN->hEventWr = ev.hEventWr;
	evN->ptrClass = this;
	if (h.hThreadReading == NULL) {
		h.hThreadReading = CreateThread(NULL,
			0,
			[](LPVOID param) -> DWORD {
				EventHandles* ev1 = (EventHandles*)param;
				DWORD res = NULL;
				HANDLE waitHandles[2] = { ev1->hEventRd, ev1->hEventExRd };
				while (1) {
					DWORD dwEvent = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
					if (waitHandles[0] == NULL) {
						cout << "ev1->hEventRd is NULL! " << GetLastError() << endl;
					}
					else if (waitHandles[1] == NULL) {
						cout << "hEventExRd is NULL! " << GetLastError() << endl;
					}
					if (dwEvent == WAIT_OBJECT_0 + 1) {
						cout << "Stop reading thread " << endl;
						delete ev1;
						return 2;
					}
					else if (dwEvent == WAIT_OBJECT_0) {
						cout << "Get signal to read from client " << endl;
						ev1->ptrClass->Readfrom();
					}
				}
				delete ev1;
				return 0;	}
		, evN, 0, NULL);
	}
}

void Server::Admin(HANDLE hImitatioOfWork) {
	char code;
	if (_kbhit())
	{
		code = _getch();
		if (code == '3')
		{
			if (hImitatioOfWork != NULL) SuspendThread(hImitatioOfWork);
			WriteToDatabaseCard();
			if (hImitatioOfWork != NULL) ResumeThread(hImitatioOfWork);
		}
	}
}

bool Server::Initialize() {
	BOOL connect_pipe;
	DWORD p_input = sizeof(Client);
	DWORD p_output = sizeof(Client);
	ev.hEventAfterOperations = CreateEvent(NULL, FALSE, FALSE, L"hEventAfterOperations");
	ev.hEventWr = CreateEvent(NULL, FALSE, FALSE, L"EventWr");
	ev.hEventRd = CreateEvent(NULL, FALSE, FALSE, L"EventRd");
	ev.hEventEx = CreateEvent(NULL, FALSE, FALSE, L"hEventEx");
	ev.hEventExRd = CreateEvent(NULL, FALSE, FALSE, L"hEventExRd");
	ev.hEventExWr = CreateEvent(NULL, FALSE, FALSE, L"hEventExWr");
	ev.hAccess = CreateEvent(NULL, FALSE, FALSE, L"hAccess");
	ev.hDenied = CreateEvent(NULL, FALSE, FALSE, L"hDenied");
	ev.hPipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\Server_pipe"
		, PIPE_ACCESS_DUPLEX
		, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_ACCEPT_REMOTE_CLIENTS
		, PIPE_UNLIMITED_INSTANCES
		, p_output
		, p_input
		, NMPWAIT_WAIT_FOREVER
		, NULL);
	if (ev.hPipe == INVALID_HANDLE_VALUE) {
		std::cout << "Error with pipe " << GetLastError() << std::endl;
		return 0;
	}
	else {
		std::cout << "========= Server start =========" << std::endl;
	}
	connect_pipe = ConnectNamedPipe(ev.hPipe, NULL);
	return connect_pipe;
}
void Server::CloseHandles() {
	cout << "Closing all handles " << endl;
	bool FlushFile;
	bool disconect_pipe;

	bool CloseHandleThreadWrite = FALSE;
	bool CloseHandleAddtodatabase = FALSE;
	bool CloseHandleImitatioOfWork = FALSE;
	bool CloseHandleThreadReading = FALSE;
	bool CloseHandleAdmin = FALSE;
	bool CloseHandleThreadExit = FALSE;

	if (h.hAddtodatabase != NULL) WaitForSingleObject(h.hAddtodatabase, INFINITE);
	if (h.hThreadWriting != NULL) WaitForSingleObject(h.hThreadWriting, INFINITE);
	if (h.hThreadReading != NULL) WaitForSingleObject(h.hThreadReading, INFINITE);

	if (h.hThreadWriting != NULL) {
		CloseHandleThreadWrite = CloseHandle(h.hThreadWriting);
		if (!CloseHandleThreadWrite) {
			cout << "Can't close handle of hThreadWriting " << GetLastError() << endl;
		}
	}
	if (h.hAddtodatabase != NULL) {
		CloseHandleAddtodatabase = CloseHandle(h.hAddtodatabase);
		if (!CloseHandleAddtodatabase) {
			cout << "Can't close handle of hAddtodatabase " << GetLastError() << endl;
		}
	}

	if (h.hImitatioOfWork != NULL) {
		CloseHandleImitatioOfWork = CloseHandle(h.hImitatioOfWork);
		if (!CloseHandleImitatioOfWork) {
			cout << "Can't close handle of hImitatioOfWork " << GetLastError() << endl;
		}
	}

	if (h.hThreadReading != NULL) {
		CloseHandleThreadReading = CloseHandle(h.hThreadReading);
		if (!CloseHandleThreadReading) {
			cout << "Can't close handle of hThreadReading " << GetLastError() << endl;
		}
	}

	if (h.hAdmin != NULL) {
		CloseHandleAdmin = CloseHandle(h.hAdmin);
		if (!CloseHandleAdmin) {
			cout << "Can't close handle of hAdmin " << GetLastError() << endl;
		}
	}

	if (h.hThreadExit != NULL) {
		CloseHandleThreadExit = CloseHandle(h.hThreadExit);
		if (!CloseHandleThreadExit) {
			cout << "Can't close handle of hThreadExit " << GetLastError() << endl;
		}
	}

	FlushFile = FlushFileBuffers(ev.hPipe);
	if (!FlushFile) cout << "Problem with flushing " << GetLastError() << endl;
	disconect_pipe = DisconnectNamedPipe(ev.hPipe);
	if (!disconect_pipe) {
		cout << "Can't disconnect named pipe " << GetLastError() << endl;
	}

	if (ev.hPipe != NULL) CloseHandle(ev.hPipe);
	if (ev.hEventWr != NULL) CloseHandle(ev.hEventWr);
	if (ev.hEventRd != NULL) CloseHandle(ev.hEventRd);
	if (ev.hEventEx != NULL) CloseHandle(ev.hEventEx);
	if (ev.hAccess != NULL) CloseHandle(ev.hAccess);
	if (ev.hDenied != NULL) CloseHandle(ev.hDenied);
	if (ev.hEventExRd != NULL) CloseHandle(ev.hEventExRd);
	if (ev.hEventExWr != NULL) CloseHandle(ev.hEventExWr);
	if (ev.hEventAfterOperations != NULL) CloseHandle(ev.hEventAfterOperations);
	cout << "All handles closed " << endl;
}


bool Server::GetConnectStatus() const {
	return connect_pipe;
}
bool Server::GetWorkStatus() const {
	return stopWork;
}

DWORD ImitatioOfWork(LPVOID) {
	while (1) {
		Sleep(700);
		std::cout << "Working..." << std::endl;
	}
}