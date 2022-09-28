
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "Packet.h"
#include "Utility.h"

#ifdef USE_GUI
	#include "sGui.h"
	void AddBtn(Widget_t* widget, void* dummy);			// Prototypes for widget's callback functions
	void GetBtn(Widget_t* widget, void* dummy);
	void RemoveBtn(Widget_t* widget, void* dummy);
	void LoginBtn(Widget_t* widget, void* dummy);

	void BtnOnHover(Widget_t* widget, void* dummy);
	void BtnOnOutHover(Widget_t* widget, void* dummy);

	Widget_t* nameField = NULL;					// Global variables to store widgets
	Widget_t* numField = NULL;
	Widget_t* outText = NULL;
	Widget_t* userText = NULL;
#else

#endif

int InitializeSocket(const char* address, const char* portNum, struct addrinfo** addrStruct);
int SendPacket(Packet_t* pack);
int ReceivePacket(Packet_t* pack);

int AddContact(char* name, char* number, char* response);
int GetContact(char* name, char* response);
int RemoveContact(char* name, char* response);
int Login(char* username, char* password, char* response);

int clientSock = -1;
struct addrinfo* serverAddr = NULL;				// Struct that contains server's address 
char username[MAX_NAME_SIZE];					// Username used to make request to server

int main(void)
{
	//strncpy(username, "user", MAX_NAME_SIZE);		// Set default username
	strncpy(username, "admin", MAX_NAME_SIZE); // ==== DEBUG ====

	if(InitializeSocket(LOCAL_ADDRESS, SERVER_PORT_NUM, &serverAddr) == 0)
		exit(-1);

	#ifdef USE_GUI
	GraphicContext_t* mainCntxt = sgui_OpenGraphicContext();
	if(mainCntxt == NULL)
		exit(-1);

	Window_t* mainWnd = sgui_CreateWindow(mainCntxt, "Phonebook", 100, 100, 480, 480, SGUI_WHITE, SGUI_WHITE);
	if(mainWnd == NULL)
		exit(-1);

	// Create all widgets
	sgui_CreateWidget(mainWnd, SGUI_BUTTON, "getBtn", 16, 400, 100, 50, "Get contact", 	sgui_Rgb(0, 120, 0), sgui_Rgb(0, 120, 0), SGUI_WHITE, SGUI_WHITE, GetBtn, BtnOnHover, BtnOnOutHover, NULL);
	sgui_CreateWidget(mainWnd, SGUI_BUTTON, "addBtn", 132, 400, 100, 50, "Add contact", 	sgui_Rgb(0, 120, 0), sgui_Rgb(0, 120, 0), SGUI_WHITE, SGUI_WHITE, AddBtn, BtnOnHover, BtnOnOutHover, NULL);
	sgui_CreateWidget(mainWnd, SGUI_BUTTON, "remBtn", 248, 400, 100, 50, "remove contact", 	sgui_Rgb(0, 120, 0), sgui_Rgb(0, 120, 0), SGUI_WHITE, SGUI_WHITE, RemoveBtn, BtnOnHover, BtnOnOutHover, NULL);
	sgui_CreateWidget(mainWnd, SGUI_BUTTON, "logBtn", 364, 400, 100, 50, "Login", 		sgui_Rgb(0, 120, 0), sgui_Rgb(0, 120, 0), SGUI_WHITE, SGUI_WHITE, LoginBtn, BtnOnHover, BtnOnOutHover, NULL);

	nameField = sgui_CreateWidget(mainWnd, SGUI_TEXTFIELD, "nameField", 100, 20, 300, 60, "", SGUI_BLACK, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);
	numField = sgui_CreateWidget(mainWnd, SGUI_TEXTFIELD, "numField", 100, 100, 300, 60, "", SGUI_BLACK, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);

	sgui_CreateWidget(mainWnd, SGUI_TEXT, "nameText", 50, 40, 40, 25, "Name:", 	SGUI_WHITE, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);
	sgui_CreateWidget(mainWnd, SGUI_TEXT, "numText", 40, 120, 50, 25, "Number:", 	SGUI_WHITE, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);
	userText = sgui_CreateWidget(mainWnd, SGUI_TEXT, "usernameText", 20, 222, 440, 40, "Logged as: user", SGUI_BLACK, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);
	outText = sgui_CreateWidget(mainWnd, SGUI_TEXT, "outText", 20, 280, 440, 40, "Output: ", 	SGUI_BLACK, SGUI_WHITE, SGUI_BLACK, SGUI_BLACK, NULL, NULL, NULL, NULL);

	while(sgui_IsWindowActive(mainWnd) == 1)		// Until window is open
	{
		sgui_UpdateWindow(mainWnd);			// Update window
	}

	sgui_DestroyWindow(mainWnd);
	sgui_CloseGraphicContext(mainCntxt);

	#else
	char commandBuff[MAX_NAME_SIZE];	
	char nameBuff[MAX_NAME_SIZE];
	char numBuff[MAX_PHONE_NUM_SIZE];
	char response[MAX_RESPONSE_SIZE];

	do {
		printf("=======[ Phonebook client ]=======\n1] Add contact\n2] Get contact\n");
		printf("3] Remove contact\n4] Login\n5] Quit\n %s ==> ", username);
		fgets(commandBuff, MAX_NAME_SIZE, stdin);

		switch(commandBuff[0])
		{
			case '1':				// Send request to add a new contact
			RETRY_ADD_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);
				printf("Insert phone number: ");
				fgets(numBuff, MAX_PHONE_NUM_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';
				numBuff[strlen(numBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0 || IsNumberValid(numBuff, MAX_PHONE_NUM_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c and number cannot contain letters... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_ADD_CONTACT;
				}

				AddContact(nameBuff, numBuff, response);
				printf("Server: %s\n", response);
				break;

			case '2':				// Send request to retrive a phone number
			RETRY_GET_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';
				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_GET_CONTACT;
				}

				GetContact(nameBuff, response);
				printf("Server: %s\n", response);
				break;

			case '3':				// Send request to remove a contact
			RETRY_REMOVE_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';
				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_REMOVE_CONTACT;
				}

				RemoveContact(nameBuff, response);
				printf("Server: %s\n", response);
				break;

			case '4':				// Send request to login
			RETRY_LOGIN:
				printf("Insert username: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);
				printf("Insert password: ");
				fgets(numBuff, MAX_PHONE_NUM_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';
				numBuff[strlen(numBuff) - 1] = '\0';
				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0 || IsNumberValid(numBuff, MAX_PASSWORD_SIZE) == 0)
				{
					printf("Username cannot contain %c or %c, password cannot contain letters or more than 8 chars... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_LOGIN;
				}

				Login(nameBuff, numBuff, response);
				printf("Server: %s\n", response);
				break;

			case '5':				// Quit
				printf("Quitting...\n");
				break;

			case '6':
				printf("\e[2J");
				break;

			default:
				printf("Unrecognized command...\n");
				break;
		}

	} while(commandBuff[0] != '5');
	#endif

	freeaddrinfo(serverAddr);
	close(clientSock);
	return 0;
}


// Allocates an addrinfo struct, fills it with info about the address and creates a UDP socket to communicate with such address
int InitializeSocket(const char* address, const char* portNum, struct addrinfo** addrStruct)
{
	// Check if something has already been initialized or if there are missing parameters
	if(clientSock != -1 || address == NULL || portNum == NULL || addrStruct == NULL || *addrStruct != NULL)
		return 0;

	printf("Intializing socket... ");
	*addrStruct = malloc(sizeof(struct addrinfo));
	if(*addrStruct == NULL)
	{
		fprintf(stderr, "Error: cannot allocate memory to hold server's address\n");
		return 0;
	}

	struct addrinfo addrHints;				// Set properties that server's address should have
	memset(&addrHints, 0, sizeof(struct addrinfo));
	memset(serverAddr, 0, sizeof(struct addrinfo));
	addrHints.ai_family = AF_INET;
	addrHints.ai_protocol = 0;
	addrHints.ai_socktype = SOCK_DGRAM;

	if(getaddrinfo(address, portNum, &addrHints, addrStruct) != 0)	// Try to get server's address
	{
		fprintf(stderr, "Error: getaddrinfo() failed\n");
		return 0;
	}

	clientSock = socket((*addrStruct)->ai_family, (*addrStruct)->ai_socktype, (*addrStruct)->ai_protocol);
	if(clientSock == -1)
	{
		fprintf(stderr, "Error: cannot create socket for client\n");
		freeaddrinfo(serverAddr);
		return 0;
	}

	struct timeval timeout;					// Set timeout of 10 second for receive operations
	timeout.tv_sec = 10;

	if(setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) != 0)
	{
		fprintf(stderr, "Warning: cannot set timeout for receive operations... ");
	}

	printf("Socket ready!\n");
	return 1;
}


// Uses clientSock to send given packet to serverAddr
int SendPacket(Packet_t* pack)
{
	if(clientSock == -1 || serverAddr == NULL || pack == NULL)
		return 0;

	int bytesSent = 0;
	bytesSent = sendto(clientSock, pack, sizeof(Packet_t), 0, serverAddr->ai_addr, serverAddr->ai_addrlen);

	if(bytesSent != sizeof(Packet_t))			// Check that all packet has been sent
	{
		printf("Failed to send request...\n"); // ==== DEBUG ====
		return 0;
	}

	printf("Request sent!\n");// ==== DEBUG ====
	return 1;
}


// Use clientSock to receive a packet from server and store it's content in given packet
int ReceivePacket(Packet_t* pack)
{
	if(clientSock == -1 || serverAddr == NULL || pack == NULL)
		return 0;

	errno = 0;
	size_t bytesReceived = 0;
	bytesReceived = recv(clientSock, pack, sizeof(Packet_t), 0);

	if(errno == EAGAIN)					// If timeout occurred write an error message in pack
	{
		snprintf(pack->name, MAX_NAME_SIZE, "Server is not responding...");
		return 0;
	}

	if(bytesReceived != sizeof(Packet_t))			// Check that a complete packet has been received
	{
		printf("Failed to receive response...\n");// ==== DEBUG ====
		return 0;
	}

	printf("Received a response from server\n");// ==== DEBUG ====
	return 1;
}


// Creates a packet that request to add a new contact with given parameters, and then send it to server
int AddContact(char* name, char* number, char* response)
{
	Packet_t request;
	Packet_t serverResponse;
	memset(&request, 0, sizeof(Packet_t));

	request.type = ADD_CONTACT,				// Set packet type
	strncpy(request.clientName, username, MAX_NAME_SIZE);	// Copy client name in packet
	strncpy(request.name, name, MAX_NAME_SIZE);		// Copy contact name in packet
	strncpy(request.number, number, MAX_PHONE_NUM_SIZE);	// Copy contact number in packet
	
	if(SendPacket(&request) == 0)				// Try to send request to server
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to send request...");
		return 0;
	}

	if(ReceivePacket(&serverResponse) == 0)			// Try to receive a packet
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to receive response...");
		return 0;
	}

	snprintf(response, MAX_RESPONSE_SIZE, "%s", serverResponse.name);
	return 1;
}


// Creates a packet that request to get number of a contact with given name, and then send it to server
int GetContact(char* name, char* response)
{
	Packet_t request;
	Packet_t serverResponse;
	memset(&request, 0, sizeof(Packet_t));

	request.type = GET_CONTACT,				// Set packet type
	strncpy(request.clientName, username, MAX_NAME_SIZE);	// Copy client name in packet
	strncpy(request.name, name, MAX_NAME_SIZE);		// Copy contact name in packet

	if(SendPacket(&request) == 0)				// Try to send request to server
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to send request...");
		return 0;
	}

	if(ReceivePacket(&serverResponse) == 0)			// Try to receive a packet
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to receive response...");
		return 0;
	}

	if(serverResponse.type == ACCEPTED)
	{
		snprintf(response, MAX_RESPONSE_SIZE, "name: %s, number: %s", serverResponse.name, serverResponse.number);
		return 1;
	}

	snprintf(response, MAX_RESPONSE_SIZE, "%s", serverResponse.name);
	return 0;
}


// Creates a packet that request to remove the contact with given name, and then send it to server
int RemoveContact(char* name, char* response)
{
	Packet_t request;
	Packet_t serverResponse;
	memset(&request, 0, sizeof(Packet_t));

	request.type = REMOVE_CONTACT,						// Set packet type
	strncpy(request.clientName, username, MAX_NAME_SIZE);			// Copy client name in packet
	strncpy(request.name, name, MAX_NAME_SIZE);				// Copy contact name in packet
	
	if(SendPacket(&request) == 0)						// Try to send request to server
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to send request...");
		return 0;
	}

	if(ReceivePacket(&serverResponse) == 0)					// Try to receive a packet
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to receive response");
		return 0;
	}

	snprintf(response, MAX_RESPONSE_SIZE, "%s", serverResponse.name);	// Copy server's response in response string
	return 1;
}


// Creates a packet that request to login  with given parameters, and then send it to server
int Login(char* user, char* password, char* response)
{
	Packet_t request;
	Packet_t serverResponse;
	memset(&request, 0, sizeof(Packet_t));

	request.type = LOGIN;					// Set packet type
	strncpy(request.clientName, username, MAX_NAME_SIZE);	// Copy client name in packet
	strncpy(request.name, user, MAX_NAME_SIZE);		// Copy user in packet
	strncpy(request.number, password, MAX_PASSWORD_SIZE);	// Copy client's password in packet
	
	if(SendPacket(&request) == 0)				// Try to send request to server
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to send request...");
		return 0;
	}

	if(ReceivePacket(&serverResponse) == 0)			// Try to receive a packet
	{
		snprintf(response, MAX_RESPONSE_SIZE, "Failed to receive response...");
		return 0;
	}

	if(serverResponse.type == ACCEPTED)			// If login request has been accepted
	{
		strncpy(username, user, MAX_NAME_SIZE); 	// Set new username
		snprintf(response, MAX_RESPONSE_SIZE, "Logged as %s", username);
		return 1;
	}

	snprintf(response, MAX_RESPONSE_SIZE, "%s", serverResponse.name);
	return 0;
}


#ifdef USE_GUI

// Called when user clicks on add contact button
void AddBtn(Widget_t* widget, void* dummy)
{
	if(IsNameValid(sgui_GetWidgetText(nameField), MAX_NAME_SIZE) == 0 || IsNumberValid(sgui_GetWidgetText(numField), MAX_PHONE_NUM_SIZE) == 0)
	{
		Widget_t* outText = sgui_GetWidget(widget->parent, "outText");
		sgui_SetWidgetText(outText, "Name or number contains invalid chars!", 1);
		return;
	}

	AddContact(sgui_GetWidgetText(nameField), sgui_GetWidgetText(numField), sgui_GetWidgetText(outText));
	sgui_RedrawWidget(outText);
}


// Called when user clicks on add contact button
void GetBtn(Widget_t* widget, void* dummy)
{
	if(IsNameValid(sgui_GetWidgetText(nameField), MAX_NAME_SIZE) == 0)
	{
		Widget_t* outText = sgui_GetWidget(widget->parent, "outText");
		sgui_SetWidgetText(outText, "Name contains invalid chars or is too long!", 1);
		return;
	}

	GetContact(sgui_GetWidgetText(nameField), sgui_GetWidgetText(outText));
	sgui_RedrawWidget(outText);
}


// Called when user clicks on add contact button
void RemoveBtn(Widget_t* widget, void* dummy)
{
	if(IsNameValid(sgui_GetWidgetText(nameField), MAX_NAME_SIZE) == 0)
	{
		sgui_SetWidgetText(outText, "Name contains invalid chars or is too long!", 1);
		return;
	}

	RemoveContact(sgui_GetWidgetText(nameField), sgui_GetWidgetText(outText));
	sgui_RedrawWidget(outText);
}


// Called when user clicks on add contact button
void LoginBtn(Widget_t* widget, void* dummy)
{
	if(IsNameValid(sgui_GetWidgetText(nameField), MAX_NAME_SIZE) == 0 || IsNumberValid(sgui_GetWidgetText(numField), MAX_PASSWORD_SIZE) == 0)
	{
		sgui_SetWidgetText(outText, "Username/password contains invalid chars or is too long!", 1);
		return;
	}

	if(Login(sgui_GetWidgetText(nameField), sgui_GetWidgetText(numField), sgui_GetWidgetText(outText)) == 1)
		sgui_SetWidgetText(userText, sgui_GetWidgetText(outText), 1);

	sgui_RedrawWidget(outText);
}


// Called when user clicks on add contact button
void BtnOnHover(Widget_t* widget, void* dummy)
{
	sgui_SetWidgetColor(widget, sgui_Rgb(0, 200, 0), sgui_Rgb(0, 200, 0), SGUI_WHITE, SGUI_WHITE);
}


// Called when user clicks on add contact button
void BtnOnOutHover(Widget_t* widget, void* dummy)
{
	sgui_SetWidgetColor(widget, sgui_Rgb(0, 120, 0), sgui_Rgb(0, 120, 0), SGUI_WHITE, SGUI_WHITE);
}

#endif
