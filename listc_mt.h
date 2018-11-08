/*
 * listc_mt.h
 *
 *  Created on: Oct 31, 2018
 *      Author: yoram
 */

#ifndef LISTC_MT_H_
#define LISTC_MT_H_

#include <pthread.h>

#define LISTC_INODE_MAX_ELEMENTS 0x7fffffff
#define LISTC_MAX_ELEMENTS 0x7fffffffffffffff

typedef struct _lnode
{
	int				elements;		// Actual number of elements
	void 			*arr;			// Pointer to array of elements. Element can be of any type!
	struct _lnode	*next;			// Pointer to next node in the global list
} lnode;

typedef struct _list
{
	int				maxElements;	// Max number of elements in one lnode
	int				sizeOfElement;	// Size of an element in bytes
	int				optimize;		// used to optimize insertElement()
	pthread_mutex_t insert_mtx;		// mutex for thread synchronization
	lnode 			*first;			// Point to first elements lnode
	lnode 			*last;			// Points to last elelemnts lnode
} listHDR;

typedef struct _gnext
{
	int				nextElement;	// Used when scaning list for retrieving elements
	lnode			*current;		// Used when scaning list for retrieving elements
} gnext;

typedef listHDR *LIST;

LIST createList (const int initElements, const int sizeOfElement);
void deleteList (LIST *list);
void insertElement (const LIST list, const void *element);
long long int listElements (const LIST list, const void (*cfunc)(void *));
int getNextElement (const LIST list, gnext *gnext_s, void *val);
void resetList (const LIST list, gnext *gnext_s);

#endif /* LISTC_MT_H_ */
