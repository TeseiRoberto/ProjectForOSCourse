
// This file contains definition of the binary serach tree data structure

#ifndef BST_H
#define BST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Constants.h"

typedef struct _BstNode {
	char name[MAX_NAME_SIZE];
	char number[MAX_PHONE_NUM_SIZE];
	size_t offset;

	struct _BstNode* father;
	struct _BstNode* leftChild;
	struct _BstNode* rightChild;
} BstNode_t;


BstNode_t* AddNode(BstNode_t** root, const char* name, const char* number, size_t offset);
void DeleteNode(BstNode_t** node);
void DeleteSubtree(BstNode_t* root);
void DeleteTree(BstNode_t** root);
BstNode_t* SearchNode(BstNode_t* root, const char* name);

BstNode_t* GetMin(BstNode_t* root);
BstNode_t* GetMax(BstNode_t* root);
void PrintTree(BstNode_t* root);

#endif
