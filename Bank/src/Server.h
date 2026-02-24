#ifndef SERVER_H
#define SERVER_H
#include "Libs.h"
class Server;

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
	HANDLE hEventAfterOperations = NULL;
	HANDLE hAccess = NULL;
	HANDLE hDenied = NULL;
	Server* ptrClass = NULL;
	HANDLE hEventExRd = NULL;
	HANDLE hEventExWr = NULL;
};
struct Client {
	unsigned int pos;
	double balance;
	unsigned int pin;
	char card_num[17];
};
class Server {
private:
	Handles h;
	EventHandles ev;
	Client k;
	bool connect_pipe;
	bool stopWork = false;
	unsigned int keyToencrypt = 13;
public:
	Server() {
		connect_pipe = Initialize();
	}
	void incrypt(char* card_num, int size);

	void decrypt(char* card_num, int size);

	bool WriteToDatabaseCard();

	bool WriteToDatabase(Client& k);

	bool ReadFromDatabase(Client& k);
	//чтение из базыданных и запись в пайп
	void WriteTo();
	//Проверка доступа + чтение
	void Readfrom();
	void ExitTread();
	void WriteThread();
	void ReadThread();
	void Admin(HANDLE hImitatioOfWork);
	bool Initialize();
	void CloseHandles();
	bool GetConnectStatus() const;
	bool GetWorkStatus() const;
	~Server() {
		CloseHandles();
	}
};
DWORD ImitatioOfWork(LPVOID);

#endif 