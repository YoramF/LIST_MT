/*
 * listc_mt.c
 *
 *  Created on: Oct 31, 2018
 *      Author: yoram
 *
 *  Simple unsorted list.
 *  Provide sequantial insert, and list/retrieve functions.
 *  The list can store any variable type.
 *  For the list() function a callback function should be provided.
 *  This callback function will be called for each of the stored values
 *
 *  THis version is multi thread safe .i.e. multiple threads can access shared list safly
 *  There is one critial location that need to be synchronized:
 *  1 - insert new element to LIST
 *
 *  In thise version, getNextElement() & resetList() will use private counters and pointers per calling thread
 *
 */

#include "listc_mt.h"

#include <stdlib.h>		// for malloc()
#include <string.h>		// for memcpy()
#include <stdio.h>		// for pritnf()
#include <pthread.h>	// for mutex() services


#define MINLIST 100
typedef LIST lnodePTR;


// Add new lnode to existing list
static lnode *addNode (const LIST list)
{
	lnode *lnodePtr;
	void *ptr;

	// allocate the array of elements
	// no need to ZERO allocated RAM - use malloc() which is faster than calloc()
	if ((ptr = malloc(list->maxElements * list->sizeOfElement)) == NULL)
		return NULL;

	// allocate the lnode
	if ((lnodePtr = (lnode *)malloc(sizeof(lnode))) != NULL)
	{
		lnodePtr->elements = 0;
		lnodePtr->next = NULL;
		lnodePtr->arr = ptr;
	}
	else
	{
		free(ptr);
		return NULL;
	}

	// check of list is not empty, otherwise this is the first lnode in the list
	if (list->last != NULL)
		list->last->next = lnodePtr;
	else
		list->first = lnodePtr;

	list->last = lnodePtr;
	return lnodePtr;
}


// Initialize a list of elements
// inintElelemnts - minimal number of elements in the list
// sizeOfElements - size of an element in the list in bytes
LIST createList (const int initElements, const int sizeOfElement)
{
	LIST list = NULL;
	int elements;

	elements = (initElements > MINLIST? initElements : MINLIST);

	// allocate List Header record
	if ((list = malloc(sizeof(listHDR))) == NULL)
		return NULL;

	// Initialize list header
	list->maxElements = elements;
	list->sizeOfElement = sizeOfElement;
	list->first = NULL;
	list->last = NULL;
	list->optimize = (sizeOfElement < 9? sizeOfElement: 9);

	// initialize mutex
	if (pthread_mutex_init(&(list->insert_mtx), NULL) != 0)
	{
		free (list);
		return NULL;
	}

	// allocate the initial lnode in the list
	if (addNode(list) == NULL)
	{
		free(list);
		return NULL;
	}


	return list;
}

// Release all memory taken by list
void deleteList (LIST *list)
{
	lnode *lptr, *lptrn;

	// Scan all lnodes, release memory pointed by *arr and then relese lnode itself
	lptr = (*list)->first;
	while (lptr != NULL)
	{
		lptrn = lptr->next;
		if (lptr->arr != NULL)
			free(lptr->arr);
		free(lptr);
		lptr = lptrn;
	}

	// Release mutex
	pthread_mutex_destroy(&((*list)->insert_mtx));

	free(*list);
	*list = NULL;
}


void insertElement (const LIST list, const void *element)
{
	lnode *lnodePtr;
	void *arrPtr;

	// Lock list for new element insertion
	if (pthread_mutex_lock(&(list->insert_mtx)) != 0)
		return;

	 lnodePtr = list->last;

	// Check if new element can't be added to current lnode
	if (lnodePtr->elements >= list->maxElements)
		lnodePtr = addNode(list);

	arrPtr = lnodePtr->arr + (lnodePtr->elements)*list->sizeOfElement;
	switch (list->optimize)
	{
	case 1:
		*(char *)arrPtr = *(char *)element;
		break;
	case 2:
		*(unsigned short *)arrPtr = *(unsigned short *)element;
		break;
	case 4:
		*(unsigned long *)arrPtr = *(unsigned long *)element;
		break;
	case 8:
		*(unsigned long long *)arrPtr = *(unsigned long long *)element;
		break;
	default:
		memcpy(arrPtr, element, list->sizeOfElement);
	}


	// increment nunber of elements in lnode
	lnodePtr->elements++;

	// Release lock
	pthread_mutex_unlock(&(list->insert_mtx));
}

// List all elements in LIST using callback function
// Since list can be used with any type variables, the calling module must provide the right print function
// for the stored variable
// The right/functional way to list stored element is by using resetList() and getNextElement()
long long int listElements (const LIST list, const void (*cfunc)(void *))
{
	long long int elements = 0;
	int i;
	lnode *lnodePtr = list->first;
	void *elementPtr;

	// Scan all lnodes in the list
	while (lnodePtr != NULL)
	{
		// point to first element in lnode
		elementPtr = lnodePtr->arr;

		for (i = 0; i < lnodePtr->elements; i++)
		{
			// increment total number of listed elements
			elements++;

			// call the callback function with the cuttent value
			cfunc(elementPtr);

			// point to next element in lnode
			elementPtr += list->sizeOfElement;
		}

		// Advance to next lnode
		lnodePtr = lnodePtr->next;
	}

	return elements;
}

// Get next element stored in list
// Before first call to getNextElement, we must call resetList()
int getNextElement (const LIST list, gnext *gnext_s, void *val)
{
	void *ePtr;

	// First check if next element is within current lnode
	if (gnext_s->nextElement >= list->maxElements)
	{
		gnext_s->current = gnext_s->current->next;
		gnext_s->nextElement = 0;
	}

	// Fetch next element only if current pouints to valid lnode
	if (gnext_s->current != NULL)
		if (gnext_s->nextElement < gnext_s->current->elements)
		{
			ePtr = gnext_s->current->arr + (gnext_s->nextElement * list->sizeOfElement);
			gnext_s->nextElement++;

			switch (list->optimize)
			{
			case 1:
				*(char *)val = *(char *)ePtr;
				break;
			case 2:
				*(unsigned short *)val = *(unsigned short *)ePtr;
				break;
			case 4:
				*(unsigned long *)val = *(unsigned long *)ePtr;
				break;
			case 8:
				*(unsigned long long *)val = *(unsigned long long *)ePtr;
				break;
			default:
				memcpy(val, ePtr, list->sizeOfElement);
			}

			return 1;
		}

	return 0;
}

// Must be called once before looping on list and calling getNextElement
void resetList (const LIST list, gnext *gnext_s)
{
	gnext_s->nextElement = 0;
	gnext_s->current = list->first;
}
