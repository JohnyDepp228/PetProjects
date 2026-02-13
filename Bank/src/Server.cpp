#include "Libs.h"

struct Handles {
	HANDLE hThreadReading = NULL;
	HANDLE hThreadWriting = NULL;
	HANDLE hThreadExit = NULL;
	HANDLE hAddtodatabase = NULL;
	HANDLE hImitatioOfWork = NULL;
	HANDLE hAdmin = NULL;
};
struct EventHandles {
	HANDLE hPipe = NULL;
	HANDLE hEventWr = NULL;
	HANDLE hEventRd = NULL;
	HANDLE hEventEx = NULL;
	HANDLE hAccess = NULL;
	HANDLE hDenied = NULL;
};
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
void WriteTo(const EventHandles& ev) {
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
void Readfrom(EventHandles& ev) {
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
	read_pipe = ReadFile(ev.hPipe, &k1, sizeof(Client), &read_Bytes, NULL);
	if (read_pipe) {
		if (read_Bytes != sizeof(Client)) {
			std::cout << "Read less bytes " << read_Bytes << " " << GetLastError() << std::endl;
			if (hdatabase != NULL) CloseHandle(hdatabase);
			exit(1);
		}
		SetFilePointer(hdatabase, 0, NULL, FILE_BEGIN);
		while (ReadFile(hdatabase, &k2, sizeof(Client), &readbytes, NULL)) {
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

void ExitTread(const EventHandles& temp, Handles& h) {
	EventHandles* ev = new EventHandles;
	ev->hAccess = temp.hAccess;
	ev->hPipe = temp.hPipe;
	ev->hDenied = temp.hDenied;
	ev->hEventEx = temp.hEventEx;
	ev->hEventRd = temp.hEventRd;
	ev->hEventWr = temp.hEventWr;
	if (h.hThreadExit == NULL) {
		h.hThreadExit = CreateThread(NULL,
			0,
			[](LPVOID param) -> DWORD {
				EventHandles* ev1 = (EventHandles*)param;
				DWORD res = NULL;
				if (ev1->hEventEx != NULL) res = WaitForSingleObject(ev1->hEventEx, INFINITE);
				if (res == WAIT_OBJECT_0) {
					cout << "Get signal to exit from client " << endl;
				}
				else {
					cout << "No sig get form client to exit " << endl;
				}
				exit(1);
				return 0;
			}, ev, 0, NULL);
	}
}

void WriteThread(EventHandles& temp) {
	DWORD id;
	EventHandles* ev = new EventHandles;
	ev->hAccess = temp.hAccess;
	ev->hPipe = temp.hPipe;
	ev->hDenied = temp.hDenied;
	ev->hEventEx = temp.hEventEx;
	ev->hEventRd = temp.hEventRd;
	ev->hEventWr = temp.hEventWr;
	HANDLE hThread = CreateThread(
		NULL,
		0,
		[](LPVOID param) -> DWORD {
			EventHandles* ev1 = (EventHandles*)param;
			WriteTo(*ev1);
			return 0;
		}, ev, 0, &id);
	if (hThread == NULL) { std::cout << "Error with thread " << GetLastError() << std::endl; }

	std::cout << "Thread with id " << id << " start writing" << std::endl;

	if (hThread != NULL) WaitForSingleObject(hThread, INFINITE);
	std::cout << "Thread with id " << id << " end" << std::endl;
	if (hThread != NULL) CloseHandle(hThread);

	if (temp.hEventWr != NULL) { SetEvent(temp.hEventWr); }
}

void ReadThread(Handles& h, const EventHandles& temp) {
	EventHandles* ev = new EventHandles;
	ev->hAccess = temp.hAccess;
	ev->hPipe = temp.hPipe;
	ev->hDenied = temp.hDenied;
	ev->hEventEx = temp.hEventEx;
	ev->hEventRd = temp.hEventRd;
	ev->hEventWr = temp.hEventWr;
	if (h.hThreadReading == NULL) {
		h.hThreadReading = CreateThread(NULL,
			0,
			[](LPVOID param) -> DWORD {
				EventHandles* ev1 = (EventHandles*)param;
				DWORD res = NULL;
				while (1) {
					if (ev1->hEventRd != NULL) res = WaitForSingleObject(ev1->hEventRd, INFINITE);
					if (res == WAIT_OBJECT_0) {
						cout << "Get signal to read from client " << endl;
						Readfrom(*ev1);
					}
				}
				return 0;	}
		, ev, 0, NULL);
	}
}

DWORD ImitatioOfWork(LPVOID) {
	while (1) {
		Sleep(700);
		std::cout << "Working..." << std::endl;
	}
}

void Admin(HANDLE hImitatioOfWork) {
	char code;
	if (_kbhit())
	{
		code = _getch();
		if (code == '3')
		{
			if (hImitatioOfWork != NULL) SuspendThread(hImitatioOfWork);
			WriteToDatabase();
			if (hImitatioOfWork != NULL) ResumeThread(hImitatioOfWork);
		}
	}
}

bool Initialize(EventHandles& ev) {
	BOOL connect_pipe;
	DWORD p_input = sizeof(Client);
	DWORD p_output = sizeof(Client);
	ev.hEventWr = CreateEvent(NULL, FALSE, FALSE, L"EventWr");
	ev.hEventRd = CreateEvent(NULL, FALSE, FALSE, L"EventRd");
	ev.hEventEx = CreateEvent(NULL, FALSE, FALSE, L"hEventEx");
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
void CloseHandles(Handles& h, EventHandles& ev) {
	bool FlushFile;
	bool disconect_pipe;

	bool CloseHandleThreadWrite = FALSE;
	bool CloseHandleAddtodatabase = FALSE;
	bool CloseHandleImitatioOfWork = FALSE;
	bool CloseHandleThreadReading = FALSE;
	bool CloseHandleAdmin = FALSE;
	bool CloseHandleThreadExit = FALSE;

	if (h.hImitatioOfWork != NULL) WaitForSingleObject(h.hImitatioOfWork, INFINITE);
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
}

int main()
{
	BOOL connect_pipe;
	BOOL FlushFile;
	BOOL disconect_pipe;
	BOOL CloseHandleThreadWrite;
	Handles h;
	EventHandles ev;
	connect_pipe = Initialize(ev);
	if (connect_pipe) {
		ReadThread(h, ev);
		ExitTread(ev, h);
		while (1) {
			if (h.hImitatioOfWork == NULL) h.hImitatioOfWork = CreateThread(NULL, 0, ImitatioOfWork, NULL, 0, NULL);
			Admin(h.hImitatioOfWork);
		}
	}
	else {
		std::cout << "Error with connecting to PIPE " << GetLastError() << std::endl;
	}

	CloseHandles(h, ev);
	return 0;
}
