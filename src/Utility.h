
// This file contains some utility functions that are used by both server and client, in particular we have functions that checks if a given string 
// is in the right format to be handled and functions that sends/receive a Packet_t struct

#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include "Packet.h"
#include "Constants.h"

int IsNameValid(const char* name, size_t maxLength);
int IsNumberValid(const char* num, size_t maxSize);
int IsPermissionsValid(const char* perm);

int SendPacket(int sock, Packet_t* pack, struct sockaddr_in* addr, socklen_t addrLen);
int ReceivePacket(int sock, Packet_t* pack);

#endif
