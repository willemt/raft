/**
 * Copyright (c) 2013, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

typedef struct {
    /** The ID that this node used to have.
     * So that we can tell which nodes were removed/added when the
     * configuration changes */
    int old_id;

    /** User data pointer for addressing.
     * Examples of what this could be:
     * - void* pointing to implementor's networking data
     * - a (IP,Port) tuple */
    void* udata_address;
} raft_node_configuration_t;

typedef struct {
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
    /* the entry's unique ID */
    unsigned int id;

    /* entry data */
    unsigned char* data;

    /* length of entry data */
    unsigned int len;
} msg_entry_t;

typedef struct {
    /* the entry's unique ID */
    unsigned int id;

    /* whether or not the entry was committed */
    int was_committed;
} msg_entry_response_t;

typedef struct {
    /* currentTerm, for candidate to update itself */
    int term;

    /* true means candidate received vote */
    int vote_granted;
} msg_requestvote_response_t;

typedef struct {
    int term;
    int leader_id;
    int prev_log_idx;
    int prev_log_term;
    int n_entries;
    msg_entry_t* entries;
    int leader_commit;
} msg_appendentries_t;

typedef struct {
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

enum {
    RAFT_MSG_REQUESTVOTE,
    RAFT_MSG_REQUESTVOTE_RESPONSE,
    RAFT_MSG_APPENDENTRIES,
    RAFT_MSG_APPENDENTRIES_RESPONSE,
    RAFT_MSG_ENTRY,
    RAFT_MSG_ENTRY_RESPONSE,
};

typedef int (
    *func_send_f
)   (
    void *cb_ctx,
    void *udata,
    int node,
    int msg_type,
    const unsigned char *send_data,
    const int len
);

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
typedef void (
    *func_log_f
)    (
    void *cb_ctx,
    void *src,
    const char *buf,
    ...
);
#endif

/**
 * Apply this log to the state macine */
typedef int (
    *func_applylog_f
)   (
    void *cb_ctx,
    void *udata,
    const unsigned char *data,
    const int len
);

typedef struct {
    func_send_f send;
    func_log_f log;
    func_applylog_f applylog;
} raft_cbs_t;

typedef void* raft_server_t;
typedef void* raft_node_t;

typedef struct {
    /* entry's term */
    unsigned int term;
    /* the entry's unique ID */
    unsigned int id;
    /* entry data */
    unsigned char* data;
    /* length of entry data */
    unsigned int len;
    /* number of nodes that have this entry */
    unsigned int nnodes;
} raft_entry_t;

/**
 * Initialise a new raft server
 * Election timeout defaults to 1000 milliseconds */
raft_server_t* raft_new();

/**
 * De-Initialise raft server */
void raft_free(raft_server_t* me_);

/**
 * Set callbacks
 * @param funcs Callbacks
 * @param cb_ctx The context that we include with all callbacks */
void raft_set_callbacks(raft_server_t* me, raft_cbs_t* funcs, void* cb_ctx);

/**
 * Set configuration
 * @param nodes Array of nodes, end of array is marked by NULL entry
 * @param my_idx Which node is myself */
void raft_set_configuration(raft_server_t* me_,
        raft_node_configuration_t* nodes, int my_idx);

/**
 * Set election timeout
 * @param millisec Election timeout in milliseconds */
void raft_set_election_timeout(raft_server_t* me, int millisec);

/**
 * Set request timeout in milliseconds
 * @param millisec Request timeout in milliseconds */
void raft_set_request_timeout(raft_server_t* me_, int millisec);

/**
 * Run actions that are dependent on time passing
 * @return 0 on error */
int raft_periodic(raft_server_t* me, int msec_since_last_period);

/**
 * Receive an appendentries message
 * @param node Who sent us the response
 * @param ae The appendentries message 
 * @return 0 on error */
int raft_recv_appendentries(raft_server_t* me, int node,
        msg_appendentries_t* ae);

/**
 * Receive a response from an appendentries message we sent
 * @param node Who sent us the response
 * @param r The appendentries response 
 * @return 0 on error */
int raft_recv_appendentries_response(raft_server_t* me_,
        int node, msg_appendentries_response_t* r);
/**
 * Receive a requestvote message
 * @param node Who sent us the message
 * @param vr The requestvote message
 * @return 0 on error */
int raft_recv_requestvote(raft_server_t* me, int node,
        msg_requestvote_t* vr);

/**
 * Receive a response from a requestvote message we sent
 * @param node Who sent us the response
 * @param r The requestvote response 
 * @param node The node this response was sent by */
int raft_recv_requestvote_response(raft_server_t* me, int node,
        msg_requestvote_response_t* r);

/**
 * Receive an entry message from client.
 * Append the entry to the log
 * Send appendentries to followers 
 * @param node The node this response was sent by
 * @param e The entry message */
int raft_recv_entry(raft_server_t* me, int node, msg_entry_t* e);

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
 * @return index of last applied entry */
int raft_get_last_applied_idx(raft_server_t* me);

/**
 * @return 1 if node is leader; 0 otherwise */
int raft_node_is_leader(raft_node_t* node);

/**
 * @return the node's next index */
int raft_node_get_next_idx(raft_node_t* node);

/**
 * @param idx The entry's index
 * @return entry from index */
raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int idx);

/**
 * @param node The node's index
 * @return node pointed to by node index
 */
raft_node_t* raft_get_node(raft_server_t *me_, int node);

/**
 * @return number of votes this server has received this election */
int raft_get_nvotes_for_me(raft_server_t* me_);

/**
 * @return node ID of who I voted for */
int raft_get_voted_for(raft_server_t* me);

