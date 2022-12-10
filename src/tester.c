

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "Constants.h"
#include "Utility.h"
#include "Packet.h"

typedef struct _Tester {
	pthread_t tid;			// Id of the thread used to simulate 
	int sock;			// Socket used by tester to communicate
	Packet_t request;		// Request that tester will send to server
	Packet_t response;		// Response that tester will receive from server
} Tester_t;


int InitializeSocket(int* sock, struct sockaddr_in* addr);
int GetAddressStruct(const char* ipAddress, const char* portNum, struct sockaddr_in* addr);

int InitializeTesters(Tester_t** testers, int threadNum, int getReqNum, int addReqNum, int removeReqNum);
void DestroyTesters(Tester_t** testers, int testerNum);
void* SimulateRequest(void* index);
void PrintResults(Tester_t* tester, int testerNum);


// Array of strings that will be used when simulating GET_CONTACT requests
char* toSearch[] = { "Giuseppe", "Anna", "Gianni", "Luca"};

// Array of strings that will be used when simulating ADD_CONTACT requests
char* toAdd[] = { "Max", "Riccardo", "Laura", "Andrea" };

// Array of strings that will be used when simulating REMOVE_CONTACT requests
char* toRemove[] = { "Ugo", "Federico", "Alessandra", "Miriam"};

Tester_t* testers = NULL;		// Array of Tester_t structs
struct sockaddr_in serverAddr;		// Struct that contains server's address
sem_t startSem;				// Semaphore to enable begin of simulation

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		fprintf(stderr, "usage is: %s <get request num> <add request num> <remove request num>\n", argv[0]);
		exit(-1);
	}

	int getReqNum = (int) strtol(argv[1], NULL, 10);	// Num of tester that will simulate a GET_CONTACT request
	int addReqNum = (int) strtol(argv[2], NULL, 10);	// Num of tester that will simulate a ADD_CONTACT request
	int removeReqNum = (int) strtol(argv[3], NULL, 10);	// Num of tester that will simulate a REMOVE_CONTACT request
	int testersNum = getReqNum + addReqNum + removeReqNum;

	// Check that hardcoded arrays (toSearch, toAdd and toRemove) have enough elements to simulate requests
	if(getReqNum > (int)(sizeof(toSearch) / sizeof(char*)) || addReqNum > (int)(sizeof(toAdd) / sizeof(char*)) || removeReqNum > (int)(sizeof(toRemove) / sizeof(char*)))
	{
		fprintf(stderr, "Error: one of the given request num is grather than elements in the string array that is used to simulate requests,");
		fprintf(stderr, "please edit source code, add elements to the array and recompile tester.c\n");
		exit(-1);
	}

	if(GetAddressStruct(SERVER_ADDRESS, SERVER_PORT_NUM, &serverAddr) == 0)		// Fill serverAddr struct
		exit(-1);

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


// Retrives server's address and fills the struct given with it
int GetAddressStruct(const char* ipAddress, const char* portNum, struct sockaddr_in* addr)
{
	if(ipAddress == NULL || portNum == NULL || addr == NULL)
	{
		return 0;
	}

	struct addrinfo addrHints;						// Set properties that server's address should have
	struct addrinfo* serverAddress;
	memset(&addrHints, 0, sizeof(struct addrinfo));
	addrHints.ai_family = AF_INET;
	addrHints.ai_protocol = 0;
	addrHints.ai_socktype = SOCK_DGRAM;
	addrHints.ai_flags = 0;

	if(getaddrinfo(ipAddress, portNum, &addrHints, &serverAddress) != 0)	// Try to get server's address
	{
		fprintf(stderr, "Error: getaddrinfo() failed, cannot retrive server's address...\n");
		return 0;
	}

	memcpy(addr, serverAddress->ai_addr, sizeof(struct sockaddr_in));
	return 1;
}


// Creates a new socket for UDP communication
int InitializeSocket(int* sock, struct sockaddr_in* addr)
{
	if(*sock != -1 || addr == NULL)
		return 0;

	*sock = socket(addr->sin_family, SOCK_DGRAM, 0);
	if(*sock == -1)
	{
		fprintf(stderr, "Error: cannot create socket for tester...\n");
		printf("WHY: %s\n", strerror(errno));
		return 0;
	}

	struct timeval timeout;					// Set timeout of 10 second for receive operations
	timeout.tv_sec = 10;

	setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)); 
	return 1;
}


// Allocates an array of Tester_t with testerNum elements and initializes all of them, also initialize a semaphre to synch testers
int InitializeTesters(Tester_t** testers, int testerNum, int getReqNum, int addReqNum, int removeReqNum)
{
	if(testers == NULL || *testers != NULL || testerNum == 0)
		return 0;

	printf("Intializing testers... ");
	fflush(stdout);

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

	int getReqMade = 0;		// Keeps track of how many GET_CONTACT request we made (NOTE: is used as index for toSearch string array)
	int addReqMade = 0;		// Keeps track of how many ADD_CONTACT request we made (NOTE: is used as index for toAdd string array)
	int removeReqMade = 0;		// Keeps track of how many REMOVE_CONTACT request we made (NOTE: is used as index for toRemove string array)

	for(int i = 0; i < testerNum; i++)				// For each tester in the array
	{
		Tester_t* curr = &( (*testers)[i] );
		memset(&curr->request, 0, sizeof(Packet_t));			// Set request and response packet to default value
		memset(&curr->response, 0, sizeof(Packet_t));
		curr->sock = -1;
		strncpy(curr->request.clientName, "admin", MAX_NAME_SIZE);	// Set clientName in request as "admin" so that it has all permissions
		
		if(InitializeSocket(&curr->sock, &serverAddr) == 0)	// Create a socket
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

		// This switch is used to set the request type for the current Tester_t, in particular we want to send requests in order such as
		// GET, ADD and REMOVE so that we test the server capability in handling read and write request in a mixed order
		switch(i % 3)
		{
			case 0:
				TRY_ANOTHER_REQUEST:
				if(getReqMade < getReqNum)
				{
					curr->request.type = GET_CONTACT;
					strncpy(curr->request.name, toSearch[getReqMade], MAX_NAME_SIZE);
					getReqMade++;
					break;
				}
				// Fallthrough
			case 1:
				if(addReqMade < addReqNum)
				{
					curr->request.type = ADD_CONTACT;
					strncpy(curr->request.name, toAdd[addReqMade], MAX_NAME_SIZE);		// Set name in packet
					strncpy(curr->request.number, "1234567890", MAX_PHONE_NUM_SIZE);	// Set number in packet
					addReqMade++;
					break;
				}
				// Fallthrough
			case 2:
				if(removeReqMade < removeReqNum)
				{
					curr->request.type = REMOVE_CONTACT;
					strncpy(curr->request.name, toRemove[removeReqMade], MAX_NAME_SIZE);	// Set name in packet
					removeReqMade++;
					break;
				}
				goto TRY_ANOTHER_REQUEST;
				break;
		
		}
	}

	printf("Testers are ready!\n");
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

	if(SendPacket(me->sock, &me->request, &serverAddr, sizeof(serverAddr)) == 0)
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
				printf("Thread %d] Request was: ( GET_CONTACT, %s ) Response is ( %s, %s )\n", i + 1, testers[i].request.name, testers[i].response.name, testers[i].response.number);
				break;

			case ADD_CONTACT:
				printf("Thread %d] Request was: ( ADD_CONTACT, %s, %s ) Reponse is: ( %s )\n", i + 1, testers[i].request.name, testers[i].request.number, testers[i].response.name);
				break;

			case REMOVE_CONTACT:
				printf("Thread %d] Request was: ( REMOVE_CONTACT, %s ) Reponse is: ( %s )\n", i + 1, testers[i].request.name, testers[i].response.name);
				break;

			default:
				printf("Thread %d] Request was ( UNDEFINED )\n", i + 1);
				break;
		}
	}
}

