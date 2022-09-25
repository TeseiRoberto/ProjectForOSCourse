
// This file contains definition of the Packet_t struct, this is the fundamental data type exchanged between clients and server to communicate

#ifndef PACKET_H
#define PACKET_H

#include "Constants.h"

typedef enum { ADD_CONTACT, GET_CONTACT, REMOVE_CONTACT, LOGIN, ACCEPTED, REJECTED } RequestType_t;

typedef struct _Packet {
	RequestType_t type;
	char name[MAX_NAME_SIZE];
	char number[MAX_PHONE_NUM_SIZE];
	char clientName[MAX_NAME_SIZE];
} Packet_t;

#endif
