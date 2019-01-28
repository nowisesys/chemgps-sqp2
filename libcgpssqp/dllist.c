/* SIMCA-QP predictions for the ChemGPS project.
 *
 * Copyright (C) 2007-2018 Anders Lövgren and the Computing Department,
 * Uppsala Biomedical Centre, Uppsala University.
 * 
 * Copyright (C) 2018-2019 Anders Lövgren, Nowise Systems
 * ----------------------------------------------------------------------
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ----------------------------------------------------------------------
 *  Contact: Anders Lövgren <andlov@nowise.se>
 * ----------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include "dllist.h"

/*
 * Intilize the double linked list.
 */
void dllist_init(struct dllist *list, void (*destroy)(void *), int (*compare)(void *, void *))
{
	list->destroy = destroy;
	list->compare = compare;
	
	list->head = list->tail = list->curr = NULL;
	list->count = 0;
}

/*
 * Find data in list by iterating thru the list and calling
 * compare on each element.
 */
void * dllist_find(struct dllist *list, void *data, int (*compare)(void *, void *), int seek)
{
	void *curr = NULL;
	
	if(list->count == 0) {
		return NULL;
	}
	if(!data) {
		if(!list->data) {
			return NULL;
		}
		data = list->data;
	}
	if(!compare) {
		if(!list->compare) {
			return NULL;
		}
		compare = list->compare;
	}

	if(seek == 0) {
		seek = DLL_SEEK_START;
	}
	
	if(seek & DLL_SEEK_HEAD) {
		curr = dllist_move_first(list);
	} else if(seek & DLL_SEEK_TAIL) {
		curr = dllist_move_last(list);
	} else if(seek & DLL_SEEK_CURR) {
		if(!list->curr) {
			if(seek & DLL_SEEK_NEXT) {
				curr = dllist_move_first(list);
			} else if(seek & DLL_SEEK_PREV) {
				curr = dllist_move_last(list);
			}
		} else {
			if(seek & DLL_SEEK_NEXT) {
				curr = dllist_move_next(list);
			} else if(seek & DLL_SEEK_PREV) {
				curr = dllist_move_prev(list);
			}
		}
	}
	while(curr) {
		if(compare(curr, data) == 0) {
			list->data = data;   /* save for dllist_finc_next() */
			return curr;
		}
		if(seek & DLL_SEEK_NEXT) {
			curr = dllist_move_next(list);
		} else if(seek & DLL_SEEK_PREV) {
			curr = dllist_move_prev(list);
		}
	}
	
	list->data = NULL;
	return NULL;
}

/*
 * Return data from first node (head).
 */
void * dllist_move_first(struct dllist *list)
{
	list->curr = list->head;
	if(list->curr) {
		return list->curr->data;
	}
	return NULL;
}

/*
 * Return data from last node (tail).
 */
void * dllist_move_last(struct dllist *list)
{
	list->curr = list->tail;
	if(list->curr) {
		return list->curr->data;
	}
	return NULL;
}

/*
 * Return data from next node.
 */
void * dllist_move_next(struct dllist *list)
{
	if(list->curr) {
		list->curr = list->curr->next;
	}
	if(list->curr) {
		return list->curr->data;
	}
	return NULL;       
}

/*
 * Return data from previous node.
 */
void * dllist_move_prev(struct dllist *list)
{
	if(list->curr) {
		list->curr = list->curr->prev;
	}
	if(list->curr) {
		return list->curr->data;
	}
	return NULL;
}

/*
 * Add data to the list. Use where to define where to insert.
 */
int dllist_insert(struct dllist *list, void *data, int where)
{
	struct dllnode *node;
	
	node = malloc(sizeof(struct dllnode));
	if(!node) {
		return -1;
	}
	node->prev = node->next = NULL;
	node->data = data;
	
	if(list->count == 0) {
		list->head = list->tail = node;
		list->count++;
		return 0;
	}
	if(list->curr == NULL && where == DLL_INSERT_CURR) {
		where = DLL_INSERT_TAIL;
	}
	
	if(where == DLL_INSERT_HEAD) {
		node->next = list->head;
		list->head = node;
	} else if(where == DLL_INSERT_TAIL) {
		list->tail->next = node;
		node->prev = list->tail;
		list->tail = node;
	} else if(where == DLL_INSERT_CURR) {
		node->next = list->curr->next;
		node->prev = list->curr;
		list->curr->next->prev = node;
		list->curr->next = node;
	} else {
		free(node);
		return -1;
	}
	
	list->count++;
	return 0;
}

/*
 * Remove current node from the list. Use dll_first(), dll_next() or
 * dll_find() to move the iterator pointing to current node.
 */
int dllist_remove(struct dllist *list)
{
	struct dllnode * node;
	
	if(list->curr == NULL) {
		return -1;
	}
	node = list->curr;
	list->curr = node->next;
	
	if(node->next) {
		node->next->prev = node->prev;
	} else {
		list->tail = node->prev;
	}
	if(node->prev) {
		node->prev->next = node->next;
	} else {
		list->head = node->next;
	}
	if(list->destroy) {
		list->destroy(node->data);
	}
	
	free(node);
	node = NULL;
	
	list->count--;
	return 0;
}

/*
 * Free memory allocated by the dynamic list.
 */
void dllist_free(struct dllist *list)
{
	struct dllnode *curr, *next;	
	
	if(list->count) {
		for(curr = list->head; list->count != 0; curr = next) {
			next = curr->next;
			if(list->destroy) {
				list->destroy(curr->data);
			}
			free(curr);
			list->count--;
		}
	}
}
