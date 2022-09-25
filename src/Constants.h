
// This file defines some constants values that will be used across the project

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_NAME_SIZE		64						// Max number of characters that compose a name in the phonebook
#define MAX_PHONE_NUM_SIZE	11						// Max number of characters that compose a phone number in the phonebook
#define MAX_PERMISSION_SIZE	3						// Max number of characters that compose a permission string
#define MAX_PASSWORD_SIZE	MAX_PHONE_NUM_SIZE - MAX_PERMISSION_SIZE	// Max number of characters that compose a password
#define MAX_RESPONSE_SIZE 	MAX_NAME_SIZE + MAX_PHONE_NUM_SIZE + 32		
#define SERVER_PORT_NUM		"9090"						// Port number used by the server
#define LOCAL_ADDRESS		"127.0.0.1"					// Machine's local address
#define MAX_CLIENT_NUM		4						// Max number of clients that the server can handle concurrently
#define SEPARATOR_CHAR		';'						// Character used in files to separate fields of the same entry
#define REMOVED_CHAR		'|'						// Character used in files to mark an entry as removed (canceled by a user)


// Note: in MAX_..._SIZE macros the terminator '\0' is intended to be included in that amount of bytes
// Note: Bst's nodes for credentials are composed in this way, name is the username and number is splitted in two parts, one of lenght
// MAX_PASSWORD_SIZE (which is the password) then a SEPARATOR_CHAR and then 2 bytes for permissions (in the order RW this order MUST be respected)

#endif
