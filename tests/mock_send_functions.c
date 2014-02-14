#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "linked_list_queue.h"

#include "raft.h"

typedef struct {
    void* outbox;
    void* inbox;
    void* raft;
} sender_t;

typedef struct {
    void* data;
    int len;
    /* what type of message is it? */
    int type;
    /* who sent this? */
    int sender;
} msg_t;

static sender_t** __senders = NULL;
static int __nsenders = 0;

void senders_new()
{
    __senders = NULL;
    __nsenders = 0;
}

int sender_send(raft_server_t* raft,
        void* udata,
        int peer,
        raft_message_type_e type,
        const unsigned char* data,
        int len)
{
    sender_t* me = udata;
    msg_t* m;

    m = malloc(sizeof(msg_t));
    m->type = type;
    m->len = len;
    m->data = malloc(len);
    m->sender = raft_get_nodeid(raft);
    memcpy(m->data,data,len);
    llqueue_offer(me->outbox,m);

    if (__nsenders > peer)
    {
        llqueue_offer(__senders[peer]->inbox, m);
    }

    return 0;
}

void* sender_new(void* address)
{
    sender_t* me;

    me = malloc(sizeof(sender_t));
    me->outbox = llqueue_new();
    me->inbox = llqueue_new();
    __senders = realloc(__senders,sizeof(sender_t*) * (++__nsenders));
    __senders[__nsenders-1] = me;
    return me;
}

void* sender_poll_msg_data(void* s)
{
    sender_t* me = s;
    msg_t* msg;

    msg = llqueue_poll(me->outbox);
    return NULL != msg ? msg->data : NULL;
}

void sender_set_raft(void* s, void* r)
{
    sender_t* me = s;
    me->raft = r;
}

int sender_msgs_available(void* s)
{
    sender_t* me = s;

    return 0 < llqueue_count(me->inbox);
}

void sender_poll_msgs(void* s)
{
    sender_t* me = s;
    msg_t* m;

    while ((m = llqueue_poll(me->inbox)))
    {
        switch (m->type)
        {
            case RAFT_MSG_APPENDENTRIES:
                raft_recv_appendentries(me->raft, m->sender, m->data);
                break;
            case RAFT_MSG_APPENDENTRIES_RESPONSE:
                raft_recv_appendentries_response(me->raft, m->sender, m->data);
                break;
            case RAFT_MSG_REQUESTVOTE:
                raft_recv_requestvote(me->raft, m->sender, m->data);
                break;
            case RAFT_MSG_REQUESTVOTE_RESPONSE:
                raft_recv_requestvote_response(me->raft, m->sender, m->data);
                break;
            case RAFT_MSG_ENTRY:
                raft_recv_entry(me->raft, m->sender, m->data);
                break;
            case RAFT_MSG_ENTRY_RESPONSE:
                //raft_recv_entry_response(me->raft, m->sender, m->data);
                break;

        }
    }
}

