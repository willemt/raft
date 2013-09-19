#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <assert.h>

#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_msghandler.h"

typedef struct {
    uint32_t len;
    unsigned char id;
    unsigned int bytes_read;
    unsigned int tok_bytes_read;
    union {
        msg_have_t have;
        msg_bitfield_t bitfield;
        bt_block_t block;
        msg_piece_t piece;
    };
} msg_t;

typedef struct {
    /* current message we are reading */
    msg_t msg;

    /* peer connection */
    void* pc;
} raft_connection_event_handler_t;

static void __endmsg(msg_t* msg)
{
    msg->bytes_read = 0;
    msg->id = 0;
    msg->tok_bytes_read = 0;
    msg->len = 0;
}

static int __read_uint32(
        uint32_t* in,
        msg_t *msg,
        const unsigned char** buf,
        unsigned int *len)
{
    while (1)
    {
        if (msg->tok_bytes_read == 4)
        {
            msg->tok_bytes_read = 0;
            return 1;
        }
        else if (*len == 0)
        {
            return 0;
        }

        *((unsigned char*)in + msg->tok_bytes_read) = **buf;
        msg->tok_bytes_read += 1;
        msg->bytes_read += 1;
        *buf += 1;
        *len -= 1;
    }
}

static int __read_byte(
        unsigned char* in,
        unsigned int *tot_bytes_read,
        const unsigned char** buf,
        unsigned int *len)
{
    if (*len == 0)
        return 0;

    *in = **buf;
    *tot_bytes_read += 1;
    *buf += 1;
    *len -= 1;
    return 1;
}

/**
 * create a new msg handler */
void* raft_msghandler_new(void *pc)
{
    raft_connection_event_handler_t* me;

    me = calloc(1,sizeof(raft_connection_event_handler_t));
    me->pc = pc;
    return me;
}

/**
 * Receive this much data on this step. */
void raft_msghandler_dispatch_from_buffer(void *mh, const unsigned char* buf, unsigned int len)
{
    raft_connection_event_handler_t* me = mh;
    msg_t* msg = &me->msg;

    while (0 < len)
    {
        /* read length of message (int) */
        if (msg->bytes_read < 4)
        {
            if (1 == __read_uint32(&msg->len, &me->msg, &buf, &len))
            {
                /* it was a keep alive message */
                if (0 == msg->len)
                {
                    pwp_conn_keepalive(me->pc);
                    __endmsg(&me->msg);
                }
            }
        }
        /* get message ID */
        else if (msg->bytes_read == 4)
        {
            __read_byte(&msg->id, &msg->bytes_read,&buf,&len);

            if (msg->len != 1) continue;

            switch (msg->id)
            {
            case PWP_MSGTYPE_CHOKE:
                pwp_conn_choke(me->pc);
                break;
            case PWP_MSGTYPE_UNCHOKE:
                pwp_conn_unchoke(me->pc);
                break;
            case PWP_MSGTYPE_INTERESTED:
                pwp_conn_interested(me->pc);
                break;
            case PWP_MSGTYPE_UNINTERESTED:
                pwp_conn_uninterested(me->pc);
                break;
            default: assert(0); break;
            }
            __endmsg(&me->msg);
        }
        else 
        {
            switch (msg->id)
            {
            case PWP_MSGTYPE_HAVE:
                if (1 == __read_uint32(&msg->have.piece_idx,
                            &me->msg, &buf,&len))
                {
                    pwp_conn_have(me->pc,&msg->have);
                    __endmsg(&me->msg);
                    continue;
                }

                break;
            case PWP_MSGTYPE_BITFIELD:
                {
                    unsigned char val;
                    unsigned int ii;

                    if (msg->bytes_read == 1 + 4)
                    {
                         bitfield_init(&msg->bitfield.bf, (msg->len - 1) * 8);
                    }

                    __read_byte(&val, &msg->bytes_read,&buf,&len);

                    /* mark bits from byte */
                    for (ii=0; ii<8; ii++)
                    {
                        if (0x1 == ((unsigned char)(val<<ii) >> 7))
                        {
                            bitfield_mark(&msg->bitfield.bf,
                                    (msg->bytes_read - 5 - 1) * 8 + ii);
                        }
                    }

                    /* done reading bitfield */
                    if (msg->bytes_read == 4 + msg->len)
                    {
                        pwp_conn_bitfield(me->pc, &msg->bitfield);
                        __endmsg(&me->msg);
                    }
                }
                break;
            case PWP_MSGTYPE_REQUEST:
                if (msg->bytes_read < 1 + 4 + 4)
                {
                    __read_uint32(&msg->block.piece_idx,
                            &me->msg, &buf,&len);
                }
                else if (msg->bytes_read < 1 + 4 + 4 + 4)
                {
                    __read_uint32(&msg->block.block_byte_offset,
                            &me->msg,&buf,&len);
                }
                else if (1 == __read_uint32(&msg->block.block_len,
                            &me->msg, &buf,&len))
                {
                    pwp_conn_request(me->pc, &msg->block);
                    __endmsg(&me->msg);
                }

                break;
            case PWP_MSGTYPE_CANCEL:
                if (msg->bytes_read < 1 + 4 + 4)
                {
                    __read_uint32(&msg->block.piece_idx,
                            &me->msg, &buf,&len);
                }
                else if (msg->bytes_read < 1 + 4 + 4 + 4)
                {
                    __read_uint32(&msg->block.block_byte_offset,
                            &me->msg,&buf,&len);
                }
                else if (1 == __read_uint32(&msg->block.block_len,
                            &me->msg, &buf,&len))
                {
                    pwp_conn_cancel(me->pc, &msg->block);
                    __endmsg(&me->msg);
                }
                break;
            case PWP_MSGTYPE_PIECE:
                if (msg->bytes_read < 1 + 4 + 4)
                {
                    __read_uint32(&msg->piece.block.piece_idx,
                            &me->msg, &buf,&len);
                }
                else if (msg->bytes_read < 9 + 4)
                {
                    __read_uint32(&msg->piece.block.block_byte_offset,
                            &me->msg,&buf,&len);
                }
                else
                {
                    int size;

                    size = len;

                    /* check it isn't bigger than what the message tells
                     * us we should be expecting */
                    if (size > msg->len - 1 - 4 - 4)
                    {
                        size = msg->len - 1 - 4 - 4;
                    }

                    msg->piece.data = buf;
                    msg->piece.block.block_len = size;
                    pwp_conn_piece(me->pc, &msg->piece);

                    /* if we haven't received the full piece, why don't we
                     * just split it virtually? */
                    /* shorten the message */
                    msg->len -= size;
                    msg->piece.block.block_byte_offset += size;

                    /* if we received the whole message we're done */
                    if (msg->len == 9)
                        __endmsg(&me->msg);

                    len -= size;
                    buf += size;
                }
                break;
            default:
                assert(0); break;
            }
        }
    }
}

