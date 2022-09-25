

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

int InitializeSockets(int** socketArray, int socketNum, const char* address, const char* portNum, struct addrinfo** addrStruct);
void DestroySockets(int** socketArray, int socketNum);
int SendPacket(int sock, Packet_t* pack);
int ReceivePacket(int sock, Packet_t* pack);

int InitializeTesters(pthread_t** threadArray, int threadNum);
void DestroyTesters(pthread_t** threadArray);

void* SimulateRequest(void* index);

int testersNum = 0;			// Number of threads that will simulate clients sending request to server
int getRequestNum = 0;			// Number of GET_CONTACT request that will be sent to server
int addRequestNum = 0;			// Number of ADD_CONTACT request that will be sent to server
int removeRequestNum = 0;		// Number of REMOVE_CONTACT request that will be sent to server
struct addrinfo* serverAddr = NULL;	// Struct that contains server's address

char* toSearch = "Giuseppe"			// Name used for simulated GET_CONTACT requests
char* toAdd = "Max"			// Name used for simulated ADD_CONTACT requests
char* toRemove = "Anna"			// Name used for simulated REMOVE_CONTACT requests

int* socks = NULL;			// Array of sockets
pthread_t* testers = NULL;		// Array of threads
sem_t startSem;				// Semaphore to enable begin of simulation
pthread_mutex_t requestMutx;		// Mutex used to regulate the decision of which request should a tester simulate

int main(int argc, char* argv[])
{
	if(argc != 5)
	{
		fprintf(stderr, "usage is: %s <clientsNum> <getRequestNum> <addRequestNum> <removeRequestNum>\n", argv[0]);
		exit(-1);
	}

	testersNum = (int) strtol(argv[1], NULL, 10);
	getRequestNum = (int) strtol(argv[2], NULL, 10);
	addRequestNum = (int) strtol(argv[3], NULL, 10);
	removeRequestNum = (int) strtol(argv[4], NULL, 10);

	// Check that for clients that will be simulated thre is a request type
	if((getRequestNum + addRequestNum + removeRequestNum) != testersNum)
	{
		fprintf(stderr, "Error: getRequestNum + addRequestNum + removeRequestNum must be equal to clientsNum\n");
		exit(-1);
	}

	// Allocate and initialize an array of sockets
	if(InitializeSockets(&socks, testersNum, LOCAL_ADDRESS, SERVER_PORT_NUM, &serverAddr) == 0)
		exit(-1);

	// Allocate and initialize an array of threads
	if(InitializeTesters(&testers, testersNum) == 0)
	{
		DestroySockets(&socks, testersNum);
		exit(-1);
	}

	printf("Tester will simulate %d clients making: %d \"get\", %d \"add\" and %d \"remove\" request to server\n\n", testersNum, getRequestNum, addRequestNum, removeRequestNum);

	for(int i = 0; i < testersNum; i++)			// Enable tester threads to simulate
		sem_post(&startSem);

	for(int i = 0; i < testersNum; i++)			// Wait completion of all testers
		pthread_join(testers[i], NULL);

	DestroyTesters(&testers);
	DestroySockets(&socks, testersNum);
	return 0;
}


// Allocates an array of sockets with socketNum elements and initializes all of them, also retrives and initializes serverAddress
int InitializeSockets(int** socketArray, int socketNum, const char* address, const char* portNum, struct addrinfo** addrStruct)
{
	// Check if something has already been initialized or if there are missing parameters
	if(socketArray == NULL || *socketArray != NULL || address == NULL || portNum == NULL || addrStruct == NULL || *addrStruct != NULL)
		return 0;

	printf("Intializing sockets... ");

	*addrStruct = malloc(sizeof(struct addrinfo));		// Allocate memory to hold server's address
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

	*socketArray = malloc(sizeof(int) * socketNum);		// Allocate an array of sockets
	if(*socketArray == NULL)
	{
		freeaddrinfo(*addrStruct);
		fprintf(stderr, "Error: cannot allocate array of sockets\n");
		return 0;
	}

	for(int i = 0; i < socketNum; i++)
	{
		(*socketArray)[i] = socket((*addrStruct)->ai_family, (*addrStruct)->ai_socktype, (*addrStruct)->ai_protocol);
		if((*socketArray)[i] == -1)
		{
			printf("Creation of socket failed!\n"); // DEBUG
			for(int j = 0; j < i; j++)		// Close all sockets already opened
				close((*socketArray)[j]);

			fprintf(stderr, "Error: cannot create socket for client\n");
			freeaddrinfo(serverAddr);
			free(*socketArray);
			*socketArray = NULL;
			return 0;
		}
	}

	printf("Sockets array ready!\n");
	return 1;
}


// Closes all sockets and deallocates memory
void DestroySockets(int** socketArray, int socketNum)
{
	printf("Destroying sockets\n"); // DEBUG
	
	for(int i = 0; i < socketNum; i++)
	{
		close((*socketArray)[i]);
	}

	free(*socketArray);
	*socketArray = NULL;
}


// Send given packet to server using specified socket
int SendPacket(int sock, Packet_t* pack)
{
	if(sock == -1 || serverAddr == NULL || pack == NULL)
		return 0;

	int bytesSent = 0;
	bytesSent = sendto(sock, pack, sizeof(Packet_t), 0, serverAddr->ai_addr, serverAddr->ai_addrlen);

	if(bytesSent != sizeof(Packet_t))			// Check that all packet has been sent
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

	if(bytesReceived != sizeof(Packet_t))			// Check that a complete packet has been received
	{
		return 0;
	}

	return 1;
}


// Allocates an array of threads with threadNum elements and initializes all of them
int InitializeTesters(pthread_t** threadArray, int threadNum)
{
	if(threadArray == NULL || *threadArray != NULL)
		return 0;

	printf("Intializing threads... ");

	if(sem_init(&startSem, 0, 0) != 0)			// Intialize semaphore to synh testers
	{
		fprintf(stderr, "Error: cannot initialize start semaphore\n");
		return 0;
	}

	if(pthread_mutex_init(&requestMutx, NULL) != 0)		// Initialize mutex to synch decision of which type of request should a tester simulate
	{
		fprintf(stderr, "Error: cannot initialize request mutex\n");
		sem_destroy(&startSem);
		return 0;
	}

	*threadArray = malloc(sizeof(pthread_t) * threadNum);	// Allocate an array of threads (one for each tester)
	if(*threadArray == NULL)
	{
		fprintf(stderr, "Error: cannot allocate array of threads for testers\n");
		return 0;
	}

	for(int i = 0; i < threadNum; i++)			// Create threads
	{
			if(pthread_create(&(*threadArray)[i], NULL, SimulateRequest, (void*) i) != 0)
			{
				fprintf(stderr, "Error: creation of thread for tester %d failed\n", i + 1);
				for(int j = 0; j < i; j++)	// Destroy all already created threads
					pthread_cancel((*threadArray)[j]);

				sem_destroy(&startSem);
				free(*threadArray);
				*threadArray = NULL;
				return 0;
			}
	}

	printf("Threads are ready\n");
	return 1;
}


// Deallocates memory (does not call pthread_cancel because in main we already called pthread_join so we are sure that threads returned)
void DestroyTesters(pthread_t** threadArray)
{
	if(threadArray == NULL || *threadArray == NULL)
		return;

	printf("Destroying testers\n");

	free(*threadArray);
	*threadArray = NULL;
	sem_destroy(&startSem);
}


// This function is called by each tester, it creates a request packet and sends it to the server, then waits for a response
void* SimulateRequest(void* index)
{
	int i = (int) index;
	Packet_t request, response;

	pthread_mutex_lock(&requestMutx);			// Choose wich type of request will this tester simulate
	if(addRequestNum > 0)
	{
		request.type = ADD_CONTACT;
		strncpy(request.name, toAdd, MAX_NAME_SIZE);			// Set name in packet
		strncpy(request.number, "1234567890", MAX_NAME_SIZE);		// Set number in packet
		addRequestNum--;
		printf("thread %d will simulate ADD_CONTACT request\n", i + 1);

	} else if(getRequestNum > 0)
	{
		request.type = GET_CONTACT;
		strncpy(request.name, toSearch, MAX_NAME_SIZE);			// Set name in packet
		getRequestNum--;
		printf("thread %d will simulate GET_CONTACT request\n", i + 1);

	} else if(removeRequestNum > 0)
	{
		request.type = REMOVE_CONTACT;
		strncpy(request.name, toRemove, MAX_NAME_SIZE);			// Set name in packet
		removeRequestNum--;
		printf("thread %d will simulate REMOVE_CONTACT request\n", i + 1);

	} else {
		return NULL;
	}

	pthread_mutex_unlock(&requestMutx);

	strncpy(request.clientName, "admin", MAX_NAME_SIZE);	// Set clientName in packet as "admin" so that it has all permissions

	sem_wait(&startSem);					// Wait main thread to enable simulation

	int a = SendPacket(socks[i], &request);
	int b = ReceivePacket(socks[i], &response);

	pthread_mutex_lock(&requestMutx);

	if(a == 0)
	{
		printf("Thread %d has failed to send packet...\n", i + 1);
		return NULL;
	}

	if(b == 0)
	{
		printf("Thread %d has failed to receive response...\n", i + 1);
		return NULL;
	}

	printf("Thread %d has received response, name: %s, number: %s\n", i + 1, response.name, response.number);
	pthread_mutex_unlock(&requestMutx);

	return NULL;
}
