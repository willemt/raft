/*
 
Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include "linked_list_queue.h"

void *llqueue_new()
{
    linked_list_queue_t *qu;

    qu = calloc(1, sizeof(linked_list_queue_t));
    return qu;
}

void llqueue_free(
    linked_list_queue_t * qu
)
{
    llqnode_t *node;

    node = qu->head;

    while (node)
    {
        llqnode_t *prev;

        prev = node;
        node = node->next;
        free(prev);
    }
    free(qu);
}

void *llqueue_poll(
    linked_list_queue_t * qu
)
{
    llqnode_t *node;

    void *item;

    if (qu->head == NULL)
        return NULL;

    node = qu->head;
    item = node->item;
    if (qu->tail == qu->head)
        qu->tail = NULL;
    qu->head = node->next;
    free(node);
    qu->count--;

    return item;
}

void llqueue_offer(
    linked_list_queue_t * qu,
    void *item
)
{
    llqnode_t *node;

    node = malloc(sizeof(llqnode_t));
    node->item = item;
    node->next = NULL;
    if (qu->tail)
        qu->tail->next = node;
    if (!qu->head)
        qu->head = node;
    qu->tail = node;
    qu->count++;
}

void *llqueue_remove_item(
    linked_list_queue_t * qu,
    const void *item
)
{
    llqnode_t *node, *prev;

    prev = NULL;
    node = qu->head;

    while (node)
    {
        void *ritem;

        if (node->item == item)
        {
            if (node == qu->head)
            {
                return llqueue_poll(qu);
            }
            else
            {
                prev->next = node->next;
                if (node == qu->tail)
                {
                    qu->tail = prev;
                }

                qu->count--;
                ritem = node->item;
                free(node);
                return ritem;
            }
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

void *llqueue_remove_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    int (*cmp)(const void*, const void*)
)
{
    llqnode_t *node, *prev;

    assert(cmp);
    assert(item);

    prev = NULL;
    node = qu->head;

    while (node)
    {
        void *ritem;

        if (0 == cmp(node->item,item))
        {
            if (node == qu->head)
            {
                return llqueue_poll(qu);
            }
            else
            {
                prev->next = node->next;
                if (node == qu->tail)
                {
                    qu->tail = prev;
                }

                qu->count--;
                ritem = node->item;
                free(node);
                return ritem;
            }
        }

        prev = node;
        node = node->next;
    }

    return NULL;
}

void *llqueue_get_item_via_cmpfunction(
    linked_list_queue_t * qu,
    const void *item,
    long (*cmp)(const void*, const void*)
)
{
    llqnode_t *node;

    assert(cmp);
    assert(item);

    node = qu->head;

    while (node)
    {
        if (0 == cmp(node->item,item))
        {
            return node->item;
        }

        node = node->next;
    }

    return NULL;
}

int llqueue_count(
    const linked_list_queue_t * qu
)
{
    return qu->count;
}
