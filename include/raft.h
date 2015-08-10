/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#ifndef RAFT_H_
#define RAFT_H_

typedef struct
{
    /** User data pointer for addressing.
     * Examples of what this could be:
     * - void* pointing to implementor's networking data
     * - a (IP,Port) tuple */
    void* udata_address;
} raft_node_configuration_t;

typedef struct
{
    /* candidate's term */
    int term;

    /* candidate requesting vote */
    int candidate_id;

    /* idx of candidate's last log entry */
    int last_log_idx;

    /* term of candidate's last log entry */
    int last_log_term;
} msg_requestvote_t;


typedef struct {
    void *buf;
    unsigned int len;
} msg_entry_data_t;

typedef struct
{
    /* the entry's unique ID */
    unsigned int id;

    msg_entry_data_t data;
} msg_entry_t;

typedef struct
{
    /* the entry's unique ID */
    unsigned int id;

    /* whether or not the entry was committed */
    int was_committed;
} msg_entry_response_t;

typedef struct
{
    /* currentTerm, for candidate to update itself */
    int term;

    /* true means candidate received vote */
    int vote_granted;
} msg_requestvote_response_t;

typedef struct
{
    int term;
    int leader_id;
    int prev_log_idx;
    int prev_log_term;
    int leader_commit;
    int n_entries;
    msg_entry_t* entries;
} msg_appendentries_t;

typedef struct
{
    /* currentTerm, for leader to update itself */
    int term;

    /* success true if follower contained entry matching
     * prevLogidx and prevLogTerm */
    int success;

    /* Non-Raft fields follow: */
    /* Having the following fields allows us to do less book keeping in
     * regards to full fledged RPC */
    /* This is the highest log IDX we've received and appended to our log */
    int current_idx;
    /* The first idx that we received within the appendentries message */
    int first_idx;
} msg_appendentries_response_t;

typedef void* raft_server_t;
typedef void* raft_node_t;

typedef struct
{
    /* entry's term */
    unsigned int term;
    /* the entry's unique ID */
    unsigned int id;
    /* entry data */
    unsigned char* data;
    /* length of entry data */
    unsigned int len;
    /* number of nodes that have this entry */
    unsigned int num_nodes;
} raft_entry_t;

/**
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param node The peer's ID that we are sending this message to
 * @return 0 on success */
typedef int (
*func_send_requestvote_f
)   (
    raft_server_t* raft,
    void *udata,
    int node,
    msg_requestvote_t* msg
    );

/**
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param node The peer's ID that we are sending this message to
 * @return 0 on success */
typedef int (
*func_send_appendentries_f
)   (
    raft_server_t* raft,
    void *udata,
    int node,
    msg_appendentries_t* msg
    );

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
/**
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param buf The buffer that was logged */
typedef void (
*func_log_f
)    (
    raft_server_t* raft,
    void *udata,
    const char *buf
    );
#endif

/**
 * Apply this log to the state machine
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param data Data to be applied to the log
 * @param len Length in bytes of data to be applied
 * @return 0 on success */
typedef int (
*func_applylog_f
)   (
    raft_server_t* raft,
    void *udata,
    const unsigned char *data,
    const int len
    );

/**
 * Save who we voted for to disk
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param voted_for The node we voted for
 * @return 0 on success */
typedef int (
*func_persist_int_f
)   (
    raft_server_t* raft,
    void *udata,
    const int voted_for
    );

/**
 * @param raft The Raft server making this callback
 * @param udata User data that is passed from Raft server
 * @param entry The entry that the event is happening to
 * @return 0 on success */
typedef int (
*func_logentry_event_f
)   (
    raft_server_t* raft,
    void *udata,
    raft_entry_t *entry
    );

typedef struct
{
    /* message sending */
    func_send_requestvote_f send_requestvote;
    func_send_appendentries_f send_appendentries;

    /* finite state machine application */
    func_applylog_f applylog;

    /* persistence */
    func_persist_int_f persist_vote;
    func_persist_int_f persist_term;

    /* log entry persistence */
    func_logentry_event_f log_offer;
    func_logentry_event_f log_poll;
    func_logentry_event_f log_pop;

    /* debugging - optional */
    func_log_f log;
} raft_cbs_t;

/**
 * Initialise a new Raft server
 *
 * Request timeout defaults to 200 milliseconds
 * Election timeout defaults to 1000 milliseconds
 *
 * @return newly initialised Raft server */
raft_server_t* raft_new();

/**
 * De-Initialise Raft server
 * Free all memory */
void raft_free(raft_server_t* me_);

/**
 * Set callbacks.
 * Callbacks need to be set by the user for CRaft to work.
 *
 * @param funcs Callbacks
 * @param udata "User data" - user's context that's included in a callback */
void raft_set_callbacks(raft_server_t* me, raft_cbs_t* funcs, void* udata);

/**
 * Set configuration
 * @param nodes Array of nodes. End of array is marked by NULL entry
 * @param my_idx Index of the node that refers to this Raft server */
void raft_set_configuration(raft_server_t* me_,
                            raft_node_configuration_t* nodes, int my_idx)
__attribute__ ((deprecated));

/**
 * Add peer
 *
 * NOTE: This library does not yet support membership changes.
 *  Once raft_periodic has been run this will fail.
 *
 * NOTE: The order this call is made is important.
 *  This call MUST be made in the same order as the other raft peers.
 *  This is because the node ID is assigned depending on when this call is made
 *
 * @param udata The user data for the node.
 *  This is obtained using raft_node_get_udata.
 *  Examples of what this could be:
 *  - void* pointing to implementor's networking data
 *  - a (IP,Port) tuple
 * @param is_self True if this "peer" is this server
 * @return 0 on success; otherwise -1 */
int raft_add_peer(raft_server_t* me_, void* udata, int is_self);

/**
 * Set election timeout
 * The amount of time that needs to elapse before we assume the leader is down
 * @param msec Election timeout in milliseconds */
void raft_set_election_timeout(raft_server_t* me, int msec);

/**
 * Set request timeout in milliseconds
 * The amount of time before we resend an appendentries message
 * @param msec Request timeout in milliseconds */
void raft_set_request_timeout(raft_server_t* me_, int msec);

/**
 * Process events that are dependent on time passing
 * @param msec_elapsed Time in milliseconds since the last call
 * @return 0 on success */
int raft_periodic(raft_server_t* me, int msec_elapsed);

/**
 * Receive an appendentries message
 * This function will block if it needs to append the message.
 * @param node Index of the node who sent us this message
 * @param ae The appendentries message
 * @param[out] r The resulting response
 * @return 0 on success */
int raft_recv_appendentries(raft_server_t* me, int node,
                            msg_appendentries_t* ae,
                            msg_appendentries_response_t *r);

/**
 * Receive a response from an appendentries message we sent
 * @param node Index of the node who sent us this message
 * @param r The appendentries response message
 * @return 0 on success */
int raft_recv_appendentries_response(raft_server_t* me_,
                                     int node, msg_appendentries_response_t* r);
/**
 * Receive a requestvote message
 * @param node Index of the node who sent us this message
 * @param vr The requestvote message
 * @param[out] r The resulting response
 * @return 0 on success */
int raft_recv_requestvote(raft_server_t* me, int node,
                          msg_requestvote_t* vr,
                          msg_requestvote_response_t *r);

/**
 * Receive a response from a requestvote message we sent
 * @param node Index of the node who sent us this message
 * @param r The requestvote response message
 * @param node The node this response was sent by
 * @return 0 on success */
int raft_recv_requestvote_response(raft_server_t* me, int node,
                                   msg_requestvote_response_t* r);

/**
 * Receive an entry message from client.
 * Append the entry to the log
 * Send appendentries to followers
 * This function will block if it needs to append the message.
 * Will fail:
 *  - if the server is not the leader
 * @param node Index of the node who sent us this message
 * @param[out] r The resulting response
 * @param e The entry message
 * @return 0 on success, -1 on failure */
int raft_recv_entry(raft_server_t* me, int node, msg_entry_t* e,
                    msg_entry_response_t *r);

/**
 * @return the server's node ID */
int raft_get_nodeid(raft_server_t* me_);

/**
 * @return currently configured election timeout in milliseconds */
int raft_get_election_timeout(raft_server_t* me);

/**
 * @return number of nodes that this server has */
int raft_get_num_nodes(raft_server_t* me);

/**
 * @return number of items within log */
int raft_get_log_count(raft_server_t* me);

/**
 * @return current term */
int raft_get_current_term(raft_server_t* me);

/**
 * @return current log index */
int raft_get_current_idx(raft_server_t* me);

/**
 * @return 1 if follower; 0 otherwise */
int raft_is_follower(raft_server_t* me);

/**
 * @return 1 if leader; 0 otherwise */
int raft_is_leader(raft_server_t* me);

/**
 * @return 1 if candidate; 0 otherwise */
int raft_is_candidate(raft_server_t* me);

/**
 * @return currently elapsed timeout in milliseconds */
int raft_get_timeout_elapsed(raft_server_t* me);

/**
 * @return request timeout in milliseconds */
int raft_get_request_timeout(raft_server_t* me_);

/**
 * @return index of last applied entry */
int raft_get_last_applied_idx(raft_server_t* me);

/**
 * @return 1 if node is leader; 0 otherwise */
int raft_node_is_leader(raft_node_t* node);

/**
 * @return the node's next index */
int raft_node_get_next_idx(raft_node_t* node);

/**
 * @return this node's user data */
void* raft_node_get_udata(raft_node_t* me_);

/**
 * Set this node's udata */
void raft_node_set_udata(raft_node_t* me_, void* udata);

/**
 * @param idx The entry's index
 * @return entry from index */
raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int idx);

/**
 * @param node The node's index
 * @return node pointed to by node index */
raft_node_t* raft_get_node(raft_server_t *me_, int node);

/**
 * @return number of votes this server has received this election */
int raft_get_nvotes_for_me(raft_server_t* me_);

/**
 * @return node ID of who I voted for */
int raft_get_voted_for(raft_server_t* me);

/**
 * Gets what this node thinks the node ID of the leader is.
 * @return node of what this node thinks is the valid leader;
 *   -1 if the leader is unknown */
int raft_get_current_leader(raft_server_t* me);

/**
 * @return callback user data */
void* raft_get_udata(raft_server_t* me_);

/**
 * Vote for a server
 * @param node The server to vote for */
void raft_vote(raft_server_t* me_, const int node);

/**
 * Set the current term
 * @param term The new current term */
void raft_set_current_term(raft_server_t* me_, const int term);

#endif /* RAFT_H_ */
