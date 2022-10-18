
#include "Bst.h"

// Allocates a new node, set's its properties with given parameters and returns a pointer to it
BstNode_t* AddNode(BstNode_t** root, const char* name, const char* number, size_t offset)
{
	BstNode_t* newNode = malloc(sizeof(BstNode_t));
	if(newNode == NULL)
	{
		fprintf(stderr, "Error: AddNode() failed, malloc returned NULL\n");
		return NULL;
	}

	if(name != NULL)
		memcpy(newNode->name, name, MAX_NAME_SIZE);
	else
		memset(newNode->name, '\0', MAX_NAME_SIZE);

	if(number != NULL)
		memcpy(newNode->number, number, MAX_PHONE_NUM_SIZE);
	else
		memset(newNode->number, '\0', MAX_PHONE_NUM_SIZE);

	newNode->offset = offset;
	newNode->leftChild = newNode->rightChild = NULL;

	if(*root == NULL)					// If no root is given then we are creating a new tree
	{
		*root = newNode;
		newNode->father = NULL;
	} else {						// Otherwise we are inserting a new node in an already existing tree
		BstNode_t* curr = *root;
		BstNode_t* temp;

		while(curr != NULL)				// Search a father for new node
		{
			temp = curr;
			if(name[0] <= curr->name[0])
				curr = curr->leftChild;
			else
				curr = curr->rightChild;
		}

		if(strncmp(name, temp->name, MAX_NAME_SIZE) == 0)	// If a node with same name already exist
		{
			free(newNode);					// Then add node has failed
			return NULL;
		}

		if(name[0] <= temp->name[0])
			temp->leftChild = newNode;
		else
			temp->rightChild = newNode;

		newNode->father = temp;
	}

	return newNode;
}


// Deallocates node given as parameter
void DeleteNode(BstNode_t** node)
{
	if(node == NULL || *node == NULL)
		return;

	BstNode_t* toRemove = *node;

	if(toRemove->leftChild == NULL && toRemove->rightChild == NULL)	// If node has no childs
	{
		if(toRemove->father == NULL)				// If node has no father then it's the root of the tree
		{
			free(toRemove);					// So deallocate the node
			*node = NULL;					// And set tree pointer to NULL
			return;
		} else {						// If node has a father
			if(toRemove == toRemove->father->leftChild)	// Unlink node from his father
				toRemove->father->leftChild = NULL;
			else
				toRemove->father->rightChild = NULL;

			free(toRemove);
			return;
		}

	} else if(toRemove->leftChild != NULL && toRemove->rightChild == NULL)	// If node has only a left child
	{
		BstNode_t* newNode = GetMax(toRemove->leftChild);
		strncpy(toRemove->name, newNode->name, MAX_NAME_SIZE);
		strncpy(toRemove->number, newNode->number, MAX_PHONE_NUM_SIZE);
		toRemove->offset = newNode->offset;
		DeleteNode(&newNode);
		return;

	} else if(toRemove->leftChild == NULL && toRemove->rightChild != NULL)	// If node has only a right child
	{
		BstNode_t* newNode = GetMin(toRemove->rightChild);
		strncpy(toRemove->name, newNode->name, MAX_NAME_SIZE);
		strncpy(toRemove->number, newNode->number, MAX_PHONE_NUM_SIZE);
		toRemove->offset = newNode->offset;
		DeleteNode(&newNode);
		return;

	} else {							// If node has both childs
		BstNode_t* newNode = GetMin(toRemove->rightChild);
		strncpy(toRemove->name, newNode->name, MAX_NAME_SIZE);
		strncpy(toRemove->number, newNode->number, MAX_PHONE_NUM_SIZE);
		toRemove->offset = newNode->offset;
		DeleteNode(&newNode);
		return;
	}
}


// Deallocates all nodes in the subtree that has as root the given node
void DeleteSubtree(BstNode_t* root)
{
	if(root == NULL)
		return;

	int deallocated = 0; // ==== DEBUG ====
	BstNode_t* curr = root;
	BstNode_t* temp;

	while(curr != NULL)
	{
		if(curr->leftChild != NULL)
		{
			curr = curr->leftChild;
		} else if(curr->rightChild != NULL)
		{
			curr = curr->rightChild;
		} else {
			if(curr == root)			// If curr node is root and it has no childs then we have deleted all the subtree
				break;

			temp = curr;
			curr = curr->father;

			if(curr != NULL)
			{
				if(temp == curr->leftChild)	// Unlink temp node from his father
					curr->leftChild = NULL;
				else
					curr->rightChild = NULL;
			}

			free(temp);
			deallocated++; // ==== DEBUG ====
		}
	}

	printf("DeleteSubtree() deallocated %d nodes\n", deallocated); // ==== DEBUG ====
}


// Deallocates all the node of the tree starting from root
void DeleteTree(BstNode_t** root)
{
	if(root == NULL || *root == NULL)
		return;

	if((*root)->father != NULL)				// Check that given node is the root of a tree
		return;

	DeleteSubtree(*root);
	DeleteNode(root);
}


// Searches a node with given name in the tree that has as root the given node and returns a pointer to it if one is found, null otherwise
BstNode_t* SearchNode(BstNode_t* root, const char* name)
{
	if(root == NULL || name == NULL)
		return NULL;

	BstNode_t* curr = root;
	while(curr != NULL)
	{
		if(strncmp(name, curr->name, MAX_NAME_SIZE) == 0)
			return curr;

		if(name[0] <= curr->name[0])
			curr = curr->leftChild;
		else
			curr = curr->rightChild;
	}

	return NULL;
}


// Returns node with smallest value contained in the tree that has as root the given node (smallest is to be intendeed alphabetically)
BstNode_t* GetMin(BstNode_t* root)
{
	if(root == NULL)
		return NULL;

	BstNode_t* curr = root;
	while(curr->leftChild != NULL)
	{
		curr = curr->leftChild;
	}

	return curr;
}


// Returns node with greater value contained in the tree that has as root the given node (greater is to be intendeed alphabetically)
BstNode_t* GetMax(BstNode_t* root)
{
	if(root == NULL)
		return NULL;

	BstNode_t* curr = root;
	while(curr->rightChild != NULL)
	{
		curr = curr->rightChild;
	}

	return curr;
}


// Prints to stdout the content of the tree that has as root the given node, used for debug (Warning is recursive)
void PrintTree(BstNode_t* root)
{
	if(root == NULL)
		return;

	printf("- Name: %s, Number: %s, Offset: %lu\n", root->name, root->number, root->offset);
	PrintTree(root->leftChild);
	PrintTree(root->rightChild);
}

