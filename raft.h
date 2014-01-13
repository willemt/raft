
enum {
    RAFT_STATE_NONE,
    RAFT_STATE_FOLLOWER,
    RAFT_STATE_CANDIDATE,
    RAFT_STATE_LEADER
};

typedef struct {
    /* So that we can tell which nodes were removed/added */
    int old_id;
    /* User data pointer for addressing.
     * ie. This is most likely a (IP,Port) tuple */
    void* udata_address;
} raft_node_configuration_t;

typedef struct {
    /* term candidate's term */
    int term;

    /* candidateId candidate requesting vote */
    int candidate_id;

    /* idx of candidate's last log entry */
    int last_log_idx;

    /* term of candidate's last log entry */
    int last_log_term;
} msg_requestvote_t;

/**
 * every server must send entrys in this format */
typedef struct {
    unsigned int id;
    unsigned char* data;
    unsigned int len;
} msg_entry_t;

typedef struct {
    unsigned int term;
    unsigned int id;
    unsigned char* data;
    unsigned int len;
    /* number of nodes that have this entry */
    unsigned int nnodes;
} raft_entry_t;

typedef struct {
    unsigned int id;
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

    /* Non Raft fields */
    /* Having the following fields allows us to do less book keeping in
     * regards to full fledged RPC */
    /* This is the highest log IDX we've received and appended to our log */
    int current_idx;
    /* The first idx that we received within the append entries message */
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
    void *caller,
    void *udata,
    int node,
    int type,
    const unsigned char *send_data,
    const int len
);

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
typedef void (
    *func_log_f
)    (
    void *udata,
    void *src,
    const char *buf,
    ...
);
#endif

typedef int (
    *func_applylog_f
)   (
    void *caller,
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

/**
 * Initialise a new raft server */
raft_server_t* raft_new();

/**
 * De-Initialise raft server */
void raft_free(raft_server_t* me_);

void raft_set_callbacks(raft_server_t* me, raft_cbs_t* funcs, void* caller);

void raft_election_start(raft_server_t* me);

void raft_become_leader(raft_server_t* me);

void raft_become_candidate(raft_server_t* me);

int raft_receive_append_entries(raft_server_t* me, msg_appendentries_t* ae);

/**
 * @return 0 on error */
int raft_periodic(raft_server_t* me, int msec_since_last_period);

/**
 * @return 0 on error */
int raft_recv_appendentries_response(raft_server_t* me_,
        int node, msg_appendentries_response_t* aer);

/**
 * @return 0 on error */
int raft_recv_appendentries(raft_server_t* me, int node,
        msg_appendentries_t* ae);

int raft_recv_requestvote(raft_server_t* me, int node,
        msg_requestvote_t* vr);

/**
 * @param node The node this response was sent by */
int raft_recv_requestvote_response(raft_server_t* me, int node,
        msg_requestvote_response_t* r);

void raft_execute_entry(raft_server_t* me);

void raft_set_election_timeout(raft_server_t* me, int millisec);

int raft_get_election_timeout(raft_server_t* me);

void raft_vote(raft_server_t* me, int node);

raft_node_t* raft_add_node(raft_server_t* me, int node_udata);

int raft_remove_node(raft_server_t* me, int node);

/**
 * @return number of nodes that this server has */
int raft_get_num_nodes(raft_server_t* me);

/**
 * Receive an ENTRY message.
 * Append entry to log
 * Send APPENDENTRIES to followers */
int raft_recv_entry(raft_server_t* me, int node, msg_entry_t* cmd);

/**
 * @return number of items within log */
int raft_get_log_count(raft_server_t* me);

void raft_set_current_term(raft_server_t* me,int term);

void raft_set_current_idx(raft_server_t* me,int idx);

int raft_get_current_term(raft_server_t* me);

void raft_set_current_idx(raft_server_t* me,int idx);

int raft_get_current_idx(raft_server_t* me);

int raft_is_follower(raft_server_t* me);

int raft_is_leader(raft_server_t* me);

int raft_is_candidate(raft_server_t* me);

/**
 * @return 0 on error */
int raft_send_requestvote(raft_server_t* me, int node);

void raft_send_appendentries(raft_server_t* me, int node);

void raft_send_appendentries_all(raft_server_t* me_);

/**
 * Appends entry using the current term.
 * Note: we make the assumption that current term is up-to-date
 * @return 0 if unsuccessful */
int raft_append_entry(raft_server_t* me_, raft_entry_t* c);

int raft_get_timeout_elapsed(raft_server_t* me);

void raft_set_commit_idx(raft_server_t* me, int commit_idx);

void raft_set_last_applied_idx(raft_server_t* me, int idx);

void raft_set_state(raft_server_t* me_, int state);

void raft_set_request_timeout(raft_server_t* me_, int millisec);

int raft_get_last_applied_idx(raft_server_t* me);

raft_node_t* raft_node_new(void* udata);

int raft_node_is_leader(raft_node_t* node);

int raft_node_get_next_idx(raft_node_t* node);

void raft_node_set_next_idx(raft_node_t* node, int nextIdx);

/**
 * Set configuration
 * @param nodes Array of nodes, end of array is marked by NULL entry
 * @param me_idx Which node is myself */
void raft_set_configuration(raft_server_t* me_,
        raft_node_configuration_t* nodes, int me_idx);

int raft_votes_is_majority(const int nnodes, const int nvotes);

/**
 * Apply entry at lastApplied + 1. Entry becomes 'committed'.
 * @return 1 if entry committed, 0 otherwise */
int raft_apply_entry(raft_server_t* me_);

raft_entry_t* raft_get_entry_from_idx(raft_server_t* me_, int idx);

raft_node_t* raft_get_node(raft_server_t *me_, int nodeid);

int raft_get_nodeid(raft_server_t* me_);

/**
 * @return number of votes this server has received this election */
int raft_get_nvotes_for_me(raft_server_t* me_);

/**
 * @return node ID of who I voted for */
int raft_get_voted_for(raft_server_t* me);

