
#include "Phonebook.h"

// Creates a new phonebook and loads data from filenames given as parameters
Phonebook_t* CreatePhonebook(const char* pbFilename, const char* credentialsFilename)
{
	Phonebook_t* newPb = malloc(sizeof(Phonebook_t));
	if(newPb == NULL)									// Check if allocation has failed
	{
		fprintf(stderr, "Error: cannot allocate memory to hold phonebook\n");
		return NULL;
	}


	newPb->dataTree = newPb->credentialsTree = NULL;					// Set tree's root to NULL

	newPb->dataFd = open(pbFilename, O_RDWR | O_CLOEXEC | O_CREAT, 0666);			// Open phonebook's data file
	if(newPb->dataFd == -1)
	{
		fprintf(stderr, "Error: cannot open/create \"%s\" file\n", pbFilename);
		free(newPb);
		return NULL;
	}

	newPb->credentialsFd = open(credentialsFilename, O_RDWR | O_CLOEXEC | O_CREAT, 0666);	// Open credentials file
	if(newPb->credentialsFd == -1)
	{
		fprintf(stderr, "Error: cannot open/create \"%s\" file\n", credentialsFilename);
		close(newPb->dataFd);
		free(newPb);
		return NULL;
	}

	printf("loading data from files... ");
	size_t readed = 0;
	readed = LoadPhonebookFromFile(newPb);
	printf("readed %lu bytes from %s ", readed, pbFilename);

	readed = LoadCredentialsFromFile(newPb);
	if(readed == 0)							// If credentials file has no content
		AddCredential(newPb, "admin", "0000", "RW", 0, 1);	// Add a default credential

	printf("readed %lu bytes from %s\n", readed, credentialsFilename);
	return newPb;
}


// Deallocates trees and closes file descriptors
void DestroyPhonebook(Phonebook_t** pb)
{
	if(*pb == NULL)
		return;

	DeleteTree(&((*pb)->dataTree));				// Delete trees
	DeleteTree(&((*pb)->credentialsTree));
	close((*pb)->dataFd);					// Close file descriptors
	close((*pb)->credentialsFd);

	free(*pb);						// Deallocate phonebook
	*pb = NULL;
}


// Adds a new node to the phonebook's bst and (if writeOnFile is 1) new entry is wrote to file
int AddContact(Phonebook_t* pb, const char* name, const char* number, size_t offset, int writeOnFile)
{
	if(pb == NULL || name == NULL || number == NULL)
		return 0;

	if(AddNode(&(pb->dataTree), name, number, offset) == NULL)		// Try to add a new node to bst
		return 0;

	if(writeOnFile == 1)							// If specified then write new contact on file
	{
		char newEntry[MAX_NAME_SIZE + MAX_PHONE_NUM_SIZE + 3];		// Create new entry
		sprintf(newEntry, "%s%c%s\n", name, SEPARATOR_CHAR, number);
		offset = lseek(pb->dataFd, 0, SEEK_CUR);			// Get cursor's position
		WriteEntryOnFile(pb->dataFd, newEntry);				// Write new entry on file
	}
	return 1;
}


// Removes node from phonebook's bst and sign corresponding entry in file as canceled
int RemoveContact(Phonebook_t* pb, const char* name)
{
	if(pb == NULL || name == NULL)
		return 0;

	BstNode_t* toRemove = SearchNode(pb->dataTree, name);		// Search node that we want to remove

	if(toRemove == NULL)						// If node is not present in the tree
		return 0;						// Return 0 because remove contact has failed

	if(toRemove->father == NULL)					// If node is the root of the tree
		pb->dataTree = NULL;

	RemoveEntryFromFile(pb->dataFd, name, toRemove->offset);	// Remove entry from file
	DeleteNode(&toRemove);						// Then delete the node
	return 1;
}


// Adds a new node to the credential's bst and a new entry to the file
int AddCredential(Phonebook_t* pb, const char* username, const char* password, const char* permissions, size_t offset, int writeOnFile)
{
	if(pb == NULL || username == NULL || password == NULL || permissions == NULL)
		return 0;

	if(writeOnFile == 1)							// If specified then write new credential on file
	{
		char newEntry[MAX_NAME_SIZE + MAX_PHONE_NUM_SIZE + 4];		// Create new entry
		sprintf(newEntry, "%s%c%s%c%s\n", username, SEPARATOR_CHAR, password, SEPARATOR_CHAR, permissions);
		offset = lseek(pb->credentialsFd, 0, SEEK_CUR);			// Get cursor's position
		WriteEntryOnFile(pb->credentialsFd, newEntry);			// Write new entry on file
	}

	char numberField[MAX_PHONE_NUM_SIZE];					// Concatenate password and permission (separated by '\0')
	snprintf(numberField, MAX_PHONE_NUM_SIZE, "%s%c%s", password, '\0', permissions);

	AddNode(&(pb->credentialsTree), username, numberField, offset);
	return 1;
}


// Checks if username has permission to execute the specified request
int CheckPermission(Phonebook_t* pb, const char* username, RequestType_t request)
{
	if(pb == NULL || username == NULL)
		return 0;

	if(request == LOGIN)					// Everyone has permission to login we don't need to check
		return 1;

	BstNode_t* toCheck = SearchNode(pb->credentialsTree, username);
	if(toCheck == NULL)
		return 0;

	for(size_t i = 0; i < MAX_PHONE_NUM_SIZE; i++)		// Analyze all chars in number
	{

		switch(toCheck->number[i])
		{
			case 'R':				// If R is found then this user can read from phonebook
				if(request == GET_CONTACT)
					return 1;
				break;

			case 'W':				// If W is found then this user can modify the phonebook
				if(request == ADD_CONTACT || request == REMOVE_CONTACT)
					return 1;
		}
	}

	return 0;
}


// Removes node from credential's bst and sign corresponding entry in file as canceled
int RemoveCredential(Phonebook_t* pb, const char* username)
{
	if(pb == NULL || username == NULL)
		return 0;

	BstNode_t* toRemove = SearchNode(pb->credentialsTree, username);	// Search node that we want to remove

	if(toRemove == NULL)							// If node is not present in the tree
		return 0;							// Return 0 because remove contact has failed

	if(toRemove->father == NULL)						// If node is the root of the tree
		pb->credentialsTree = NULL;

	RemoveEntryFromFile(pb->credentialsFd, username, toRemove->offset);	// Remove entry from file
	DeleteNode(&toRemove);							// Then delete the node
	return 1;
}


// Parse the file specified in pb and adds a node in the phonebook's bst for each entry in the file, returns number of chars readed
size_t LoadPhonebookFromFile(Phonebook_t* pb)
{
	if(pb == NULL || pb->dataFd == -1)
		return 0;

	char buffer[64];
	char nameBuff[MAX_NAME_SIZE];
	char numBuff[MAX_PHONE_NUM_SIZE];
	memset(nameBuff, '\0', MAX_NAME_SIZE);
	memset(numBuff, '\0', MAX_PHONE_NUM_SIZE);

	size_t bytesReaded = 0;
	size_t offset = 0;					// Keeps track of where we are in the file
	int state = 0;						// Keeps track of what we are reading (0 is name, 1 is number)
	size_t j = 0;						// Index for nameBuff and numBuff

	do {							// Until there are bytes to be readed from file
		bytesReaded = read(pb->dataFd, buffer, 64);	// Read a block of data from phonebook's file

		for(size_t i = 0; i < bytesReaded; i++)		// Parse the readed block
		{
			switch(buffer[i])
			{
				case SEPARATOR_CHAR:		// If SEPARATOR_CHAR is found then here ends the name field of this entry
					nameBuff[j] = '\0';
					state = 1;
					j = 0;
					break;

				case '\n':			// If newline is found then here ends the current entry
					numBuff[j] = '\0';
					state = 0;
					j = 0;

					if(strlen(nameBuff) != 0 && strlen(numBuff) != 0)
					{
						size_t entryOffset = offset - (strlen(nameBuff) + strlen(numBuff) + 1);
						AddContact(pb, nameBuff, numBuff, entryOffset, 0);
					}
					break;

				default:
					if(state == 0 && j < (MAX_NAME_SIZE - 1))
					{
						nameBuff[j] = buffer[i];
						j++;
					} else if(state == 1 && j < (MAX_PHONE_NUM_SIZE - 1))
					{
						numBuff[j] = buffer[i];
						j++;
					}

					break;
			}
			offset++;
		}
	} while(bytesReaded != 0);

	return offset;
}


// Parse the file specified in pb and adds a node in the credential's bst for each entry in the file, returns number of chars readed
size_t LoadCredentialsFromFile(Phonebook_t* pb)
{
	if(pb == NULL || pb->credentialsFd == -1)
		return 0;

	char buffer[64];
	char nameBuff[MAX_NAME_SIZE];
	char pswdBuff[MAX_PASSWORD_SIZE];
	char permBuff[MAX_PERMISSION_SIZE];
	memset(nameBuff, '\0', MAX_NAME_SIZE);
	memset(pswdBuff, '\0', MAX_PASSWORD_SIZE);
	memset(permBuff, '\0', MAX_PERMISSION_SIZE);

	size_t bytesReaded = 0;
	size_t offset = 0;					// Keeps track of where we are in the file
	int state = 0;						// Keeps track of what we are reading (0 is username, 1 password, 2 is permissions)
	size_t j = 0;						// Index for nameBuff and numBuff

	do {								// Until there are bytes to be readed from file
		bytesReaded = read(pb->credentialsFd, buffer, 64);	// Read a block of data from phonebook's file

		for(size_t i = 0; i < bytesReaded; i++)		// Parse the readed block
		{
			switch(buffer[i])
			{
				case SEPARATOR_CHAR:		// If SEPARATOR_CHAR is found then here ends the name/password field of this entry
					if(state == 0)
						nameBuff[j] = '\0';
					else
						pswdBuff[j] = '\0';

					state++;
					j = 0;
					break;

				case '\n':			// If newline is found then here ends the current entry
					permBuff[j] = '\0';
					state = 0;
					j = 0;

					if(strlen(nameBuff) != 0 && strlen(pswdBuff) != 0 && strlen(permBuff) != 0)
					{
						size_t entryOffset = offset - (strlen(nameBuff) + strlen(pswdBuff) + strlen(permBuff) + 2);
						AddCredential(pb, nameBuff, pswdBuff, permBuff, entryOffset, 0);
					}
					break;

				default:
					if(state == 0 && j < (MAX_NAME_SIZE - 1))
					{
						nameBuff[j] = buffer[i];
						j++;
					}
					else if(state == 1 && j < (MAX_PASSWORD_SIZE - 1))
					{
						pswdBuff[j] = buffer[i];
						j++;
					}
					else if(state == 2 && j < (MAX_PERMISSION_SIZE - 1))
					{
						permBuff[j] = buffer[i];
						j++;
					}

					break;
			}
			offset++;
		}
	} while(bytesReaded != 0);

	return offset;
}


// Inserts data in file
int WriteEntryOnFile(int file, const char* data)
{
	if(file == -1 || data == NULL)
		return 0;

	write(file, data, strlen(data));
	fsync(file);						// Be sure to write modification on disk
	return 1;
}


// Removes line that matches data from file
int RemoveEntryFromFile(int file, const char* data, size_t offset)
{
	if(file == -1 || data == NULL)
		return 0;

	char name[MAX_NAME_SIZE];

	lseek(file, offset, SEEK_SET);				// Move cursor to position at which data to be removed should be
	read(file, name, MAX_NAME_SIZE);			// Read MAX_NAME_SIZE bytes from file

	for(int i = 0; i < MAX_NAME_SIZE; i++)			// Replace SEPARATOR_CHAR with string terminator
	{
		if(name[i] == SEPARATOR_CHAR || i == (MAX_NAME_SIZE - 1))
		{
			name[i] = '\0';
			break;
		}
	}

	if(strncmp(name, data, MAX_NAME_SIZE) != 0)		// If given data and name readed from file are not the same
	{
		lseek(file, 0, SEEK_END);			// Move cursor back to end of file
		return 0;
	}

	char removed = REMOVED_CHAR;

	lseek(file, offset, SEEK_SET);
	write(file, &removed, 1);				// Set entry as canceled
	fsync(file);						// Be sure to write modification on disk
		
	lseek(file, 0, SEEK_END);				// Move cursor back to end of file
	return 1;
}


