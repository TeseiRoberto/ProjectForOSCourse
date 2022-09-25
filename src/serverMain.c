
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "Phonebook.h"
#include "Packet.h"
#include "Utility.h"


typedef struct _Worker {
	pthread_t tid;				// Id of the worker thread
	sem_t isBusy;				// Semaphore that signals to worker that it has a request to satisfy
	sem_t isFree;				// Semaphore that signals to main thread that worker can accept a new request
	Packet_t request;			// Request packet sent by client
	struct sockaddr_in clientAddr;		// Struct that contains the address of the client of which we need to satisfy request
	socklen_t addrLen;			// Length of the client address
} Worker_t;


int InitializeSocket(const char* portNum);
void SendPacket(Packet_t* pack, struct sockaddr_in* clientAddr, socklen_t addrLen);

int InitializeWorkers(int workersNum);
void DestroyWorkers(int workersNum, int threadNum, int busySemNum, int freeSemNum);
void* HandleRequest(void* index);
void Shell();

Phonebook_t* pb = NULL;				// Global instance of phonebook struct
int serverRunning = 1;				// Indicates if server is active
int serverSock = -1;
pthread_mutex_t socketMutx;			// Mutex to regulate write operations on server's socket
Worker_t workers[MAX_CLIENT_NUM];		// Workers that works to satisfy clients requests

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "usage is: %s <phonebook data filename> <credentials data filename>\n", argv[0]);
		return -1;
	}

	pb = CreatePhonebook(argv[1], argv[2]);			// Create new phonebook
	if(pb == NULL)						// Check if creation failed
		exit(-1);

	if(InitializeWorkers(MAX_CLIENT_NUM) == 0)		// Initialize all threads, data and synch mechanism
	{
		DestroyPhonebook(&pb);
		exit(-1);
	}

	if(InitializeSocket(SERVER_PORT_NUM) == 0)		// Initialize server's socket
	{
		DestroyWorkers(MAX_CLIENT_NUM, MAX_CLIENT_NUM, MAX_CLIENT_NUM, MAX_CLIENT_NUM);
		DestroyPhonebook(&pb);
		exit(-1);
	}

	// Start of debug code ==========
	struct sigaction intHandler;
	intHandler.sa_handler = Shell;
	intHandler.sa_flags = 0;
	
	sigaction(SIGINT, &intHandler, NULL);			// Set signal to open server shell
	// End of debug code ===========

	printf("\nWaiting for clients...\n");

	int i = 0;						// Keeps track of which worker will handle next request
	size_t bytesReceived = 0;
	while(serverRunning == 1)
	{
		sem_wait(&workers[i].isFree);			// Wait until current worker is ready to handle new request

		bytesReceived = recvfrom(serverSock, &workers[i].request, sizeof(Packet_t), 0, (struct sockaddr*) &(workers[i].clientAddr), &workers[i].addrLen);
		if(bytesReceived == sizeof(Packet_t))		// If we received the right amount of data
		{
			sem_post(&workers[i].isBusy);		// Signal to worker that there is a request for him
			i++;
			i %= MAX_CLIENT_NUM;
		} else {
			sem_post(&workers[i].isFree);		// If we received a wrong amount of data reset current worker semaphore
		}

	}

	DestroyWorkers(MAX_CLIENT_NUM, MAX_CLIENT_NUM, MAX_CLIENT_NUM, MAX_CLIENT_NUM);
	close(serverSock);
	DestroyPhonebook(&pb);
	return 0;
}


// Creates server's socket for UDP protocol
int InitializeSocket(const char* portNum)
{
	if(serverSock != -1 || portNum == NULL)
		return 0;

	printf("Intializing socket... ");

	struct addrinfo addrHints;
	struct addrinfo* serverAddr = NULL;

	memset(&addrHints, 0, sizeof(struct addrinfo));			// Set properties that server address should have
	addrHints.ai_family = AF_INET;
	addrHints.ai_protocol = 0;
	addrHints.ai_socktype = SOCK_DGRAM;

	if(getaddrinfo(NULL, portNum, &addrHints, &serverAddr) != 0)	// Get server address
	{
		fprintf(stderr, "Error: getaddrinfo() failed...\n");
		return 0;
	}

	serverSock = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);	// Create socket to receive requests
	if(serverSock == -1)
	{
		fprintf(stderr, "Error: cannot create socket...\n");
		freeaddrinfo(serverAddr);
		return 0;
	}

	if(bind(serverSock, serverAddr->ai_addr, serverAddr->ai_addrlen) != 0)
	{
		fprintf(stderr, "Error: cannot bind socket...\n");
		close(serverSock);
		freeaddrinfo(serverAddr);
		return 0;
	}

	freeaddrinfo(serverAddr);				// Server address is not required anymore
	printf("Socket ready!\n");
	return 1;
}


// Sends a packet to the specified client
void SendPacket(Packet_t* pack, struct sockaddr_in* clientAddr, socklen_t addrLen)
{
	if(serverSock == -1 || pack == NULL || clientAddr == NULL)
		return;

	sendto(serverSock, pack, sizeof(Packet_t), 0, (struct sockaddr*) clientAddr, addrLen);
}


// Creates all threads, initializes their data and intializes all synch mechanism needed by the server
int InitializeWorkers(int workersNum)
{
	printf("Initializing worker threads... ");

	if(pthread_mutex_init(&socketMutx, NULL) != 0)
	{
		fprintf(stderr, "Error: cannot intialize socket's mutex...\n");
		return 0;
	}

	for(int i = 0; i < workersNum; i++)						// For each worker
	{
		memset(&workers[i].clientAddr, 0, sizeof(struct sockaddr_in));		// Initialize client address struct
		workers[i].addrLen = sizeof(struct sockaddr_in);

		if(sem_init(&workers[i].isBusy, 0, 0) == -1)				// Initialize isBusy semaphore
		{
			fprintf(stderr, "Error: cannot initialize busy semaphore for worker %d...\n", i + 1);
			DestroyWorkers(i, i - 1, i - 1, i - 1);
			return 0;
		}

		if(sem_init(&workers[i].isFree, 0, 1) == -1)				// Initialize isFree semaphore
		{
			fprintf(stderr, "Error: cannot initialize free semaphore for worker %d\n", i + 1);
			DestroyWorkers(i, i - 1, i, i - 1);
			return 0;
		}

		if(pthread_create(&workers[i].tid, NULL, HandleRequest, (void*) i) != 0) // Create thread
		{
			fprintf(stderr, "Error: cannot initialize thread for worker %d...\n", i +1);
			DestroyWorkers(i, i - 1, i, i);
			return 0;
		}
	}

	printf("Workers ready!\n");
	return 1;
}


// Destroyes all synch mechanism and shutdown worker threads
void DestroyWorkers(int workersNum, int threadNum, int busySemNum, int freeSemNum)
{
	printf("Destroying workers... ");
	pthread_mutex_destroy(&socketMutx);			// Destroy socket's mutex

	for(int i = 0; i < workersNum; i++)			// For each worker
	{
		if(i < threadNum)
			pthread_cancel(workers[i].tid);		// Terminate thread

		if(i < busySemNum)
			sem_destroy(&workers[i].isBusy);	// Destroy current worker's busy semaphore
	
		if(i < freeSemNum)
			sem_destroy(&workers[i].isFree);	// Destroy current worker's free semaphore
	}

	printf("Workers destroyed!\n");
}


// Analayzes request sent by client and generates a response to it
void* HandleRequest(void* index)
{
	int i = (int) index;
	Packet_t response;
	printf("WORKER %d STARTED\n", i); // ==== DEBUG ====

	while(serverRunning == 1)
	{
		memset(&response, 0, sizeof(Packet_t));
		sem_wait(&workers[i].isBusy);								// Wait until a request arrives from main thread
		printf("THREAD %d IS HANDLING REQUEST: ", i); // ==== DEBUG ====

		if(CheckPermission(pb, workers[i].request.clientName, workers[i]. request.type) == 0)	// Check if client has permission to execute such request
		{
			strncpy(response.name, "You don't have permission", MAX_NAME_SIZE);		// If it does not send an error
			response.type = REJECTED;
			SendPacket(&response, &(workers[i].clientAddr), workers[i].addrLen);
			sem_post(&workers[i].isFree);							// Signal to main thread that we are available to process a new request
			continue;
		}

		switch(workers[i].request.type)			// If it has permission then try to satisfy the request
		{
			case ADD_CONTACT:
				printf("ADD_CONTACT, FROM: %s, NAME: %s, NUM: %s\n", workers[i].request.clientName, workers[i].request.name, workers[i].request.number);

				if(AddContact(pb, workers[i].request.name, workers[i].request.number, 0, 1) == 0)
				{
					strncpy(response.name, "Add contact failed", MAX_NAME_SIZE);
					response.type = REJECTED;
				} else {
					strncpy(response.name, "Added contact", MAX_NAME_SIZE);
					response.type = ACCEPTED;
				}
				break;

			case GET_CONTACT:
			{
				printf("GET_CONTACT, FROM: %s, NAME: %s\n", workers[i].request.clientName, workers[i].request.name);

				BstNode_t* node = SearchNode(pb->dataTree, workers[i].request.name);
				if(node == NULL)
				{
					strncpy(response.name, "Contact not found", MAX_NAME_SIZE);
					response.type = REJECTED;
				} else {
					strncpy(response.name, node->name, MAX_NAME_SIZE);
					strncpy(response.number, node->number, MAX_PHONE_NUM_SIZE);
					response.type = ACCEPTED;
				}
			}	break;

			case REMOVE_CONTACT:
				printf("REMOVE_CONTACT, FROM: %s, NAME: %s\n", workers[i].request.clientName, workers[i].request.name);

				if(RemoveContact(pb, workers[i].request.name) == 0)
				{
					strncpy(response.name, "Contact not found", MAX_NAME_SIZE);
					response.type = REJECTED;
				} else {
					strncpy(response.name, "Contact removed", MAX_NAME_SIZE);
					response.type = ACCEPTED;
				}
				break;

			case LOGIN:
			{
				printf("LOGIN, FROM: %s, NAME: %s, NUMBER: %s\n", workers[i].request.clientName, workers[i].request.name, workers[i].request.number);

				BstNode_t* node = SearchNode(pb->credentialsTree, workers[i].request.name);
				if(node == NULL)
				{
					strncpy(response.name, "Username unrecognized", MAX_NAME_SIZE);
					response.type = REJECTED;
				} else {
					if(strncmp(workers[i].request.number, node->number, MAX_PASSWORD_SIZE) == 0)
					{
						strncpy(response.name, "Logged in", MAX_NAME_SIZE);
						response.type = ACCEPTED;
					} else {
						strncpy(response.name, "Wrong password", MAX_NAME_SIZE);
						response.type = REJECTED;
					}
				}
			}	break;

			default:
				printf(" INVALID REQUEST from: %s\n", workers[i].request.clientName);
				strncpy(response.name, "Invalid request", MAX_NAME_SIZE);
				response.type = REJECTED;
				break;
		}

		pthread_mutex_lock(&socketMutx);
		SendPacket(&response, &(workers[i].clientAddr), workers[i].addrLen);		// Send response packet
		pthread_mutex_unlock(&socketMutx);

		sem_post(&workers[i].isFree);							// Signal to main thread that we are available to process a new request
	}

	return NULL;
}


// Enables user to enter commands to manage server
void Shell()
{
	char commandBuff[32];
	char nameBuff[MAX_NAME_SIZE];
	char numBuff[MAX_PHONE_NUM_SIZE];
	char permBuff[4];

	do {
		printf("\n========[ Server's shell ]========\n0] Add contact\n1] Get contact\n2] Remove contact\n3] Add credential\n");
		printf("4] Remove credential\n5] Print phonebook\n6] Print credentials\n7] Quit\n8] Quit Server\n==> ");

		fgets(commandBuff, 32, stdin);
		switch(commandBuff[0])
		{
			case '0':					// Add new contact
			RETRY_ADD_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);
				printf("Insert phone number: ");
				fgets(numBuff, MAX_PHONE_NUM_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';	// Remove '\n'
				numBuff[strlen(numBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0 || IsNumberValid(numBuff, MAX_PHONE_NUM_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c and number cannot contain letters... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_ADD_CONTACT;
				}

				if(AddContact(pb, nameBuff, numBuff, 0, 1) == 0)
					printf("Add contact failed\n");
				else
					printf("Added contact\n");
				break;

			case '1':					// Get contact
			{
			RETRY_GET_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_GET_CONTACT;
				}

				BstNode_t* node = SearchNode(pb->dataTree, nameBuff);
				if(node == NULL)
					printf("Contact not found\n");
				else
					printf("name: %s, number: %s\n", node->name, node->number);
			}	break;

			case '2':					// Remove contact
			RETRY_REMOVE_CONTACT:
				printf("Insert name: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0)
				{
					printf("Name cannot contain %c or %c... Please try again\n", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_REMOVE_CONTACT;
				}

				if(RemoveContact(pb, nameBuff) == 0)
					printf("Contact not found\n");
				else
					printf("Contact removed\n");
				break;

			case '3':					// Add credential
			RETRY_ADD_CREDENTIAL:
				printf("Insert username: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);
				printf("Insert password: ");
				fgets(numBuff, MAX_PASSWORD_SIZE, stdin);
				printf("Insert permissions: ");
				fgets(permBuff, 4, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';
				numBuff[strlen(numBuff) - 1] = '\0';
				permBuff[strlen(permBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0 || IsNumberValid(numBuff, MAX_PASSWORD_SIZE) == 0 || IsPermissionsValid(permBuff) == 0)
				{
					printf("Username cannot contain %c or %c, password cannot contain letters and permission can contain only R, W or RW...", SEPARATOR_CHAR, REMOVED_CHAR);
					printf(" Please try again\n");
					goto RETRY_ADD_CREDENTIAL;
				}

				if(AddCredential(pb, nameBuff, numBuff, permBuff, 0, 1) == 0)
					printf("Add credential failed\n");
				else
					printf("Added new credential\n");
				break;

			case '4':					// Remove credential
			RETRY_REMOVE_CREDENTIAL:
				printf("Insert username: ");
				fgets(nameBuff, MAX_NAME_SIZE, stdin);

				nameBuff[strlen(nameBuff) - 1] = '\0';

				if(IsNameValid(nameBuff, MAX_NAME_SIZE) == 0)
				{
					printf("Username cannot contain %c or %c... Please try again", SEPARATOR_CHAR, REMOVED_CHAR);
					goto RETRY_REMOVE_CREDENTIAL;
				}

				if(RemoveCredential(pb, nameBuff) == 0)
					printf("Credential not found\n");
				else
					printf("Credential removed\n");
				break;

			case '5':					// Print phonebook
				printf("\n=====[ Phonebook content ]=====\n");
				PrintTree(pb->dataTree);
				printf("=================================\n");
				break;

			case '6':					// Print credentials
				printf("\n=====[ Credentials content ]=====\n");
				PrintTree(pb->credentialsTree);
				printf("=================================\n");
				break;

			case '7':					// Quit
				printf("quitting shell...\n");
				break;

			case'8':
				printf("Quitting server...\n");
				serverRunning = 0;
				commandBuff[0] = '7';
				break;

			default:
				printf("invalid command...\n");
				break;
		}
	} while(commandBuff[0] != '7');
}


