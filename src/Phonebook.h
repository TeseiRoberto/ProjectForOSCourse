
// This file contains the definition of the Phonebook_t struct

#ifndef PHONEBOOK_H
#define PHONEBOOK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "Bst.h"
#include "Packet.h"

typedef struct _Phonebook {
	BstNode_t* dataTree;					// Bst that contains all phonebook's entries
	BstNode_t* credentialsTree;				// Bst that contains all credentials for clients
	int dataFd;						// File descriptor of file that contains phonebook's data
	int credentialsFd;					// File descriptor of file that contains credentials
} Phonebook_t;

Phonebook_t* CreatePhonebook(const char* pbFilename, const char* credentialsFilename);
void DestroyPhonebook(Phonebook_t** pb);

int AddContact(Phonebook_t* pb, const char* name, const char* number, size_t offset, int writeOnFile);
int RemoveContact(Phonebook_t* pb, const char* name);

int AddCredential(Phonebook_t* pb, const char* username, const char* password, const char* permissions, size_t offset, int writeOnFile);
int CheckPermission(Phonebook_t* pb, const char* username, RequestType_t request);
int RemoveCredential(Phonebook_t* pb, const char* username);


size_t LoadPhonebookFromFile(Phonebook_t* pb);
size_t LoadCredentialsFromFile(Phonebook_t* pb);
int WriteEntryOnFile(int file, const char* data);
int RemoveEntryFromFile(int file, const char* data, size_t offset);

#endif
