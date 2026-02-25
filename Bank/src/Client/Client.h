#ifndef CLIENT_H
#define CLIENT_H

#include "Libs.h"
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
	* This struct contains all handles
*/
struct Handles {
	HANDLE hPipe = NULL;
	HANDLE hEventRd = NULL;
	HANDLE hEventWr = NULL;
	HANDLE hEventEx = NULL;
	HANDLE hAccess = NULL;
	HANDLE hDenied = NULL;
	HANDLE hEventAfterOperations = NULL;
	HANDLE hEventExRd = NULL;
	HANDLE hEventExWr = NULL;
};
/*
	/*brief
	* Class with methods reqiured to work clinet app correctly 
*/
class ClientClass {
private:
	Client k;
	Handles h;
public:
	/*
	/*brief
	* Constructor where all handles initialize and print menu
	*/
	ClientClass() {
		Initialize();
		Menu();
	}
	/*
	/*brief
	* Function for initializing handles and checking for working server if it's not working programm ends
	*/
	void Initialize();
	/*
	/*brief
	* Function printing menu 
	*/
	void Menu();
	/*
	/*brief
	* Function for writing to pipe.Here user enter his card number and pin 
	*/
	bool WritetoPipe();
	/*
	/*brief
	* Function for creating thread for witing data to NamedPipes & sends signal to server to read from pipe
	*/
	void ThreadWrite();
	/*
	/*brief
	* Function for printig & do operations user calls 
	/* params
	* Client& k template struct for storing data changed by user
	*/
	void Operations(Client& k);
	/*
	/*brief
	* Function for closing all handles
	*/
	void CloseHandles();
	/*
	/*brief
	* Destructor where all handels closed by CloseHandles method
	*/
	~ClientClass() {
		CloseHandles();
	}
};

#endif
