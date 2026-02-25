#ifndef SERVER_H
#define SERVER_H
#include "Libs.h"
class Server;
/*
	/*brief
	* This struct contains handles which are not apply to signal handels
*/
struct Handles {
	HANDLE hThreadReading = NULL;
	HANDLE hThreadWriting = NULL;
	HANDLE hThreadExit = NULL;
	HANDLE hAddtodatabase = NULL;
	HANDLE hImitatioOfWork = NULL;
	HANDLE hAdmin = NULL;
};
/*
	/*brief
	* This struct contains handles which are apply to signal handels & pointer to class that uses in threads to hand over methods
*/
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
/*
	/*brief
	* This struct is used to save information about client 
	* filed pos uses to write struct correctly into place where it was taken & save changes after operations correctly
*/
struct Client {
	unsigned int pos;
	double balance;
	unsigned int pin;
	char card_num[17];
};
/*
	/*brief
	* Class with all methods for correct work of "server"
*/

class Server {
private:
	Handles h;
	EventHandles ev;
	Client k;
	bool connect_pipe;
	bool stopWork = false;
	unsigned int keyToencrypt = 13;
public:
	/*
	/*brief
	* Class constructor which initialize all handles and checks for correct connection
	*/
	Server() {
		connect_pipe = Initialize();
	}
	/*
	/*brief
	* Function uses for incrypting data like namber of card.Uses XOR method
	*/
	void incrypt(char* card_num, int size);
	/*
	/*brief
	* Function uses for decrypting data like namber of card.Uses XOR method
	*/
	void decrypt(char* card_num, int size);
	/*
	/*brief
	* Function uses for adding ONE card number to database.It works when "admin" press '3'
	*/
	bool WriteToDatabaseCard();
	/*
	/*brief
	* Function uses for adding multiple card numbers to database.It works with WriteTo
	/* params
	* CLient &k uses to store information which needed to be writen to database
	*/
	bool WriteToDatabase(Client& k);
	/*
	/*brief
	* Function uses for reading multiple card numbers from database.It works with ReadFrom
	/* params
	* CLient &k uses to store information which was read from database
	*/
	bool ReadFromDatabase(Client& k);
	/*
	/*brief
	* Function uses for adding struct with data about client into NamedPipes.It works with WriteThread
	*/
	void WriteTo();
	/*
	/*brief
	* Function uses for cheacking access by reading all file until the end and sends access signal or deny signal to client
	*/
	void Readfrom();
	/*
	/*brief
	* Function uses for ending of work when client ends it. Works with help of signal 
	*/
	void ExitTread();
	/*
	/*brief
	* Function creates thread which write data to pipe
	* Uses method WaitForMultipleObjects for realising when to close thread correctly
	*/
	void WriteThread();
	/*
	/*brief
	* Function creates thread which read data from pipe
	* Uses method WaitForMultipleObjects for realising when to close thread correctly
	*/
	void ReadThread();
	/*
	/*brief
	* Function for adding new card by pressing '3'
	/param HANDLE hImitatioOfWork parametp of thread what imitate work of server by writing to console "working.." uses for stoping thread to give admin opportunity to add card
	*/
	void Admin(HANDLE hImitatioOfWork);
	/*
	/*brief
	* Function initialize all handles
	*/
	bool Initialize();
	/*
	/*brief
	* Function close all handles and threads also flush file buffer
	*/
	void CloseHandles();
	bool GetConnectStatus() const;
	bool GetWorkStatus() const;
	/*
	/*brief
	* Destructor where all handles closing after work
	*/
	~Server() {
		CloseHandles();
	}
};
DWORD ImitatioOfWork(LPVOID);

#endif 