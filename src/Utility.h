
// This file contains utility functions that are used by server and client, in particular those functions checks that given string is in the right
// format to be handled

#ifndef UTILITY_H
#define UTILITY_H

#include <string.h>
#include "Constants.h"

int IsNameValid(const char* name, size_t maxLength);
int IsNumberValid(const char* num, size_t maxSize);
int IsPermissionsValid(const char* num);

#endif
