
/**
 * @file
 * @brief 
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "raft.h"

void* raft_new()
{
    raft_server_t* me;

    me = calloc(1,sizeof(raft_server_t));

    return me;
}

void raft_free(void* me_)
{
    raft_server_t* me = me_;

    free(me);
}

void raft_set_state(void* me_)
{
    raft_server_t* me = me_;

}

int raft_get_state(void* me_)
{
    raft_server_t* me = me_;

    return 0;
}

void raft_set_external_functions(void* r, raft_external_functions_t* funcs, void* caller)
{

}

void raft_election_start(void* r)
{

}

/**
 * Candidate i transitions to leader. */
void raft_become_leader(raft_server_t* me)
{

}

void raft_become_candidate(raft_server_t* me)
{

}

/**
 * Convert to candidate if election timeout elapses without either
 *  Receiving valid AppendEntries RPC, or
 *  Granting vote to candidate
 */
#if 0
int raft_election_timeout_elapsed(void* me_)
{
    raft_server_t* me = me_;

    if (nvalid_AEs_since_election == 0 || votes_granted_to_candidate_since_election == 0)
    {
        raft_become_candidate(me);
    }
}
#endif

int raft_periodic(void* me_, int msec_since_last_period)
{
    raft_server_t* me = me_;

    return 0;
}

/**
 * Invoked by leader to replicate log entries (§5.3); also used as heartbeat (§5.2). */
int raft_recv_appendentries(void* me_, void* peer, msg_appendentries_t* ae)
{
    return 0;
}

int raft_recv_appendentries_response(void* me_, void* peer, msg_appendentries_response_t* ae)
{
    return 0;
}

int raft_recv_requestvote(void* me_, void* peer, msg_requestvote_t* vr)
{
    return 0;
}

int raft_recv_requestvote_response(void* me_, void* peer, msg_requestvote_response_t* r)
{
    return 0;
}

void raft_execute_command(void* me_)
{
}

void raft_set_election_timeout(void* me_, int millisec)
{
}

int raft_get_election_timeout(void* me_)
{
    return 0;
}

int raft_vote(void* me_, void* peer)
{
    return 0;
}

#if 0
void raft_set_configuration(raft_server_t* me)
{
    return 0;
}
#endif

void* raft_add_peer(void* me_, void* peer_udata)
{
    return NULL;
}

int raft_remove_peer(void* me_, void* peer)
{
    return 0;
}

int raft_get_num_peers(void* me_)
{

    return 0;
}

int raft_recv_command(void* me_, void* peer, msg_command_t* cmd)
{
    return 0;
}

int raft_get_log_count(void* me_)
{
    return 0;
}

void raft_set_current_term(void* me_,int term)
{

}

int raft_get_current_term(void* me_)
{
    return 0;
}

void raft_set_current_index(void* me_,int idx)
{

}

int raft_get_current_index(void* me_)
{
    return 0;
}

int raft_is_follower(void* me_)
{

    return 0;
}

int raft_is_leader(void* me_)
{

    return 0;
}

int raft_is_candidate(void* me_)
{

    return 0;
}

int raft_send_requestvote(void* me_, void* peer)
{

    return 0;
}

int raft_append_command(void* me_, unsigned char* data, int len)
{
    return 0;
}
