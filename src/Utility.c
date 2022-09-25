
#include "Utility.h"

// If name contains invalid chars (chars used internally by server to represent/format string/file) returns 0, 1 otherwise
int IsNameValid(const char* name, size_t maxLength)
{
	if(name == NULL)
		return 0;

	size_t nameLen = strlen(name);
	if(nameLen == 0 || nameLen > (maxLength - 1))
		return 0;

	for(size_t i = 0; i < nameLen; i++)
	{
		if(name[i] == SEPARATOR_CHAR || name[i] == REMOVED_CHAR)
			return 0;
	}

	return 1;
}


// If number contains an invalid char (chars used internally by server to represent/format string/file) returns 0, 1 otherwise
int IsNumberValid(const char* num, size_t maxLength)
{
	if(num == NULL)
		return 0;

	size_t numLen = strlen(num);
	if(numLen == 0 || numLen > (maxLength - 1))
		return 0;

	for(size_t i = 0; i < numLen; i++)
	{
		if(num[i] < 48 || num[i] > 57)			// Check that all chars are numbers in ascii encoding
			return 0;
	}

	return 1;
}


// If perm is 'R', 'W' or 'RW' returns 1, otherwise returns 0
int IsPermissionsValid(const char* perm)
{
	if(perm == NULL)
		return 0;

	size_t permLen = strlen(perm);

	switch(permLen)
	{
		case 1:
			if(*perm == 'R' || *perm == 'W')
				return 1;
			break;

		case 2:
			if(perm[0] == 'R' && perm[1] == 'W')
				return 1;
			break;
	}

	return 0;
}

