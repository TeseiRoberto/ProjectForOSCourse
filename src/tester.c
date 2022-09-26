

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "Constants.h"
#include "Packet.h"

typedef struct _Tester {
	pthread_t tid;			// Id of the thread used to simulate 
	int sock;			// Socket used by tester to communicate
	Packet_t request;		// Request that tester will send to server
	Packet_t response;		// Response that tester will receive from server
} Tester_t;


int InitializeSocket(int* sock, const char* address, const char* portNum, struct addrinfo** addrStruct);
int SendPacket(int sock, Packet_t* pack);
int ReceivePacket(int sock, Packet_t* pack);

int InitializeTesters(Tester_t** testers, int threadNum, int getReqNum, int addReqNum, int removeReqNum);
void DestroyTesters(Tester_t** testers, int testerNum);
void* SimulateRequest(void* index);
void PrintResults(Tester_t* tester, int testerNum);


// Array of strings that will be used when simulating GET_CONTACT requests
char* toSearch[] = { "Giuseppe", "Anna", "Gianni", "Luca"};

// Array of strings that will be used when simulating GET_CONTACT requests
char* toAdd[] = { "Max", "Riccardo", "Laura", "Andrea" };

// Array of strings that will be used when simulating GET_CONTACT requests
char* toRemove[] = { "Ugo", "Federico", "Alessandra", "Miriam"};

Tester_t* testers = NULL;		// Array of Tester_t structs
struct addrinfo* serverAddr = NULL;	// Struct that contains server's address
sem_t startSem;				// Semaphore to enable begin of simulation

int main(int argc, char* argv[])
{
	if(argc != 5)
	{
		fprintf(stderr, "usage is: %s <clients num> <get request num> <add request num> <remove request num>\n", argv[0]);
		exit(-1);
	}

	int testersNum = (int) strtol(argv[1], NULL, 10);
	int getReqNum = (int) strtol(argv[2], NULL, 10);	// Num of tester that will simulate a GET_CONTACT request
	int addReqNum = (int) strtol(argv[3], NULL, 10);	// Num of tester that will simulate a ADD_CONTACT request
	int removeReqNum = (int) strtol(argv[4], NULL, 10);	// Num of tester that will simulate a REMOVE_CONTACT request

	// Check that for all clients that will be simulated thre is a request type
	if((getReqNum + addReqNum + removeReqNum) != testersNum)
	{
		fprintf(stderr, "Error: getRequestNum + addRequestNum + removeRequestNum must be equal to clientsNum\n");
		exit(-1);
	}

	// Check that hardcoded arrays (toSearch, toAdd and toRemove) have enough elements to simulate requests
	if(getReqNum > (int)(sizeof(toSearch) / sizeof(char*)) || addReqNum > (int)(sizeof(toAdd) / sizeof(char*)) || removeReqNum > (int)(sizeof(toRemove) / sizeof(char*)))
	{
		fprintf(stderr, "Error: one of the given request num is grather than elements in the string array that is used to simulate requests,");
		fprintf(stderr, "please edit source code, add elements to the array and recompile tester.c\n");
		exit(-1);
	}

	// Allocate and initialize an array of testers
	if(InitializeTesters(&testers, testersNum, getReqNum, addReqNum, removeReqNum) == 0)
		exit(-1);

	printf("Tester will simulate %d clients making: %d \"get\", %d \"add\" and %d \"remove\" request to server\n\n", testersNum, getReqNum, addReqNum, removeReqNum);

	for(int i = 0; i < testersNum; i++)			// Enable tester threads to simulate
		sem_post(&startSem);

	for(int i = 0; i < testersNum; i++)			// Wait completion of all testers
		pthread_join(testers[i].tid, NULL);

	PrintResults(testers, testersNum);			// Print results of all testers
	DestroyTesters(&testers, testersNum);
	return 0;
}


// Creates a new socket for UDP communication, also retrives and initializes serverAddress (if yet not initialized)
int InitializeSocket(int* sock, const char* address, const char* portNum, struct addrinfo** addrStruct)
{
	if(*sock != -1 || address == NULL || portNum == NULL || addrStruct == NULL)
		return 0;

	if(*addrStruct == NULL)					// If server address has not yet been initialized
	{
		*addrStruct = malloc(sizeof(struct addrinfo));	// Allocate memory to hold server's address
		if(*addrStruct == NULL)
		{
			fprintf(stderr, "Error: cannot allocate memory to hold server's address...\n");
			return 0;
		}

		struct addrinfo addrHints;			// Set properties that server's address should have
		memset(&addrHints, 0, sizeof(struct addrinfo));
		memset(serverAddr, 0, sizeof(struct addrinfo));
		addrHints.ai_family = AF_INET;
		addrHints.ai_protocol = 0;
		addrHints.ai_socktype = SOCK_DGRAM;

		if(getaddrinfo(address, portNum, &addrHints, addrStruct) != 0)	// Try to get server's address
		{
			fprintf(stderr, "Error: getaddrinfo() failed...\n");
			return 0;
		}
	}

	*sock = socket((*addrStruct)->ai_family, (*addrStruct)->ai_socktype, (*addrStruct)->ai_protocol);
	if(*sock == -1)
	{
		fprintf(stderr, "Error: cannot create socket for tester...\n");
		return 0;
	}
	return 1;
}


// Send given packet to server using specified socket
int SendPacket(int sock, Packet_t* pack)
{
	if(sock == -1 || serverAddr == NULL || pack == NULL)
		return 0;

	int bytesSent = 0;
	bytesSent = sendto(sock, pack, sizeof(Packet_t), 0, serverAddr->ai_addr, serverAddr->ai_addrlen);

	if(bytesSent != sizeof(Packet_t))			// Check that all packet is been sent
	{
		return 0;
	}

	return 1;
}


// Fills given packet with response from server
int ReceivePacket(int sock, Packet_t* pack)
{
	if(sock == -1 || serverAddr == NULL || pack == NULL)
		return 0;

	size_t bytesReceived = 0;
	bytesReceived = recv(sock, pack, sizeof(Packet_t), 0);

	if(bytesReceived != sizeof(Packet_t))			// Check that a complete packet is been received
	{
		return 0;
	}

	return 1;
}


// Allocates an array of Tester_t with testerNum elements and initializes all of them, also initialize a semaphre to synch testers
int InitializeTesters(Tester_t** testers, int testerNum, int getReqNum, int addReqNum, int removeReqNum)
{
	if(testers == NULL || *testers != NULL || testerNum == 0)
		return 0;

	printf("Intializing testers... ");
	*testers = malloc(sizeof(Tester_t) * testerNum);		// Allocate an array of Tester_t
	if(*testers == NULL)
	{
		fprintf(stderr, "Error: cannot allocate array of testers...\n");
		return 0;
	}

	if(sem_init(&startSem, 0, 0) != 0)				// Intialize semaphore to synch testers
	{
		fprintf(stderr, "Error: cannot initialize start semaphore\n");
		free(*testers);
		return 0;
	}

	int getReqMade = 0;		// Keeps track of how many GET_CONTACT request we made and is used as index for toSearch string array
	int addReqMade = 0;		// Keeps track of how many ADD_CONTACT request we made and is used as index for toAdd string array
	int removeReqMade = 0;		// Keeps track of how many REMOVE_CONTACT request we made and is used as index for toRemove string array

	for(int i = 0; i < testerNum; i++)				// For each tester in the array
	{
		Tester_t* curr = &( (*testers)[i] );
		memset(&curr->request, 0, sizeof(Packet_t));			// Set request and response packet to default value
		memset(&curr->response, 0, sizeof(Packet_t));
		curr->sock = -1;
		strncpy(curr->request.clientName, "admin", MAX_NAME_SIZE);	// Set clientName in request as "admin" so that it has all permissions
		
		if(InitializeSocket(&curr->sock, LOCAL_ADDRESS, SERVER_PORT_NUM, &serverAddr) == 0)	// Create a socket
		{
			for(int j = 0; j < i; j++)			// If create socket fails 
			{
				curr = &( (*testers)[j]);
				pthread_cancel(curr->tid);		// Quit all threads previously launched

				if(curr->sock != -1)
					close(curr->sock);		// Close all sockets previously opened
			}

			free(*testers);
			sem_destroy(&startSem);
			return 0;
		}

		if(pthread_create(&curr->tid, NULL, SimulateRequest, (void*) curr) != 0)	// Launch thread for tester
		{
			fprintf(stderr, "Error: creation of thread for tester %d failed\n", i + 1);
			for(int j = 0; j < (i + 1); j++)			// If launch thread fails 
			{
				curr = &( (*testers)[j]);

				if(j < i)
					pthread_cancel(curr->tid);		// Quit all threads previously launched

				close(curr->sock);			// Close all sockets previously opened
			}

			free(*testers);
			sem_destroy(&startSem);
			return 0;
		}

		if(getReqMade < getReqNum)					// Choose a request type to simulate with curr tester
		{
			curr->request.type = GET_CONTACT;
			strncpy(curr->request.name, toSearch[getReqMade], MAX_NAME_SIZE);	// Set name in packet
			getReqMade++;

		} else if(addReqMade < addReqNum)
		{
			curr->request.type = ADD_CONTACT;
			strncpy(curr->request.name, toAdd[addReqMade], MAX_NAME_SIZE);		// Set name in packet
			strncpy(curr->request.number, "1234567890", MAX_PHONE_NUM_SIZE);	// Set number in packet
			addReqMade++;

		} else if(removeReqMade < removeReqNum)
		{
			curr->request.type = REMOVE_CONTACT;
			strncpy(curr->request.name, toRemove[removeReqMade], MAX_NAME_SIZE);	// Set name in packet
			removeReqMade++;
		}
	}

	printf("Testers are ready\n");
	return 1;
}


// Deallocates memory and close sockets (does not call pthread_cancel because in main we already called pthread_join so we are sure that threads returned)
void DestroyTesters(Tester_t** testers, int testerNum)
{
	if(testers == NULL || *testers == NULL)
		return;

	printf("Destroying testers\n");

	for(int i = 0; i < testerNum; i++)
	{
		Tester_t* curr = &( (*testers)[i] );

		if(curr->sock != -1)
			close(curr->sock);
	}

	free(*testers);
	*testers = NULL;
	sem_destroy(&startSem);
}


// This function is called by each tester, it creates a request packet and sends it to the server, then waits for a response
void* SimulateRequest(void* tester)
{
	Tester_t* me = (Tester_t*) tester;
	sem_wait(&startSem);					// Wait main thread to enable simulation

	if(SendPacket(me->sock, &me->request) == 0)		// Try to send request
	{
		snprintf(me->response.name, MAX_NAME_SIZE, "Thread has failed to send request...\n");
		return NULL;
	}

	if(ReceivePacket(me->sock, &me->response) == 0)		// Try to receive a response
	{
		snprintf(me->response.name, MAX_NAME_SIZE, "Thread has failed to receive response...\n");
		return NULL;
	}

	return NULL;
}


// For all tester in given array prints to console the content of their response packet
void PrintResults(Tester_t* testers, int testerNum)
{
	if(testers == NULL)
		return;

	for(int i = 0; i < testerNum; i++)
	{
		switch(testers[i].request.type)
		{
			case GET_CONTACT:
				printf("Thread %d] Request: GET_CONTACT, response is name: %s, number: %s\n", i + 1, testers[i].response.name, testers[i].response.number);
				break;

			case ADD_CONTACT:
				printf("Thread %d] Request: ADD_CONTACT, reponse is: %s\n", i + 1, testers[i].response.name);
				break;

			case REMOVE_CONTACT:
				printf("Thread %d] Request: REMOVE_CONTACT, reponse is: %s\n", i + 1, testers[i].response.name);
				break;

			default:
				printf("Thread %d] Request UNDEFINED\n", i + 1);
				break;
		}
	}
}


