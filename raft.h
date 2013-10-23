
enum {
    RAFT_STATE_NONE,
    RAFT_STATE_FOLLOWER,
    RAFT_STATE_CANDIDATE,
    RAFT_STATE_LEADER
};

typedef struct {
    /* So that we can tell which peers were removed/added */
    int old_id;
    /* User data pointer for addressing.
     * ie. This is most likely a (IP,Port) tuple */
    void* udata_address;
} raft_peer_configuration_t;

typedef struct {
    /* term candidate's term */
    int term;

    /* candidateId candidate requesting vote */
    int candidate_id;

    /* index of candidate's last log entry */
    int last_log_index;

    /* term of candidate's last log entry */
    int last_log_term;
} msg_requestvote_t;

/**
 * every server must send commands in this format */
typedef struct {
    unsigned int id;
    unsigned char* data;
    unsigned int len;
} msg_command_t;

typedef struct {
    unsigned int term;
    unsigned int id;
    unsigned char* data;
    unsigned int len;
} raft_command_t;


typedef struct {
    unsigned int id;
    int was_committed;
} msg_command_response_t;

typedef struct {
    /* currentTerm, for candidate to update itself */
    int term;

    /* true means candidate received vote */
    int vote_granted;
} msg_requestvote_response_t;

typedef struct {
    int term;
    int leader_id;
    int prev_log_index;
    int prev_log_term;
    int n_entries;
    msg_command_t* entries;
    int leader_commit;
} msg_appendentries_t;

typedef struct {
    /* currentTerm, for leader to update itself */
    int term;

    /* success true if follower contained entry matching
     * prevLogIndex and prevLogTerm */
    int success;
} msg_appendentries_response_t;

typedef int (
    *func_send_f
)   (
    void *caller,
    void *udata,
    int peer,
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
} raft_external_functions_t;

typedef struct {
    int pass;
//    recv_appendentries_f recv_appendentries,
//    recv_requestvote_f recv_requestvote,
} raft_functions_t;

typedef void* raft_server_t;

typedef void* raft_peer_t;

raft_server_t* raft_new();

int raft_get_voted_for(raft_server_t* me);

void raft_set_external_functions(raft_server_t* me, raft_external_functions_t* funcs, void* caller);

void raft_election_start(raft_server_t* me);

void raft_become_leader(raft_server_t* me);

void raft_become_candidate(raft_server_t* me);

int raft_receive_append_entries(raft_server_t* me, msg_appendentries_t* ae);

int raft_periodic(raft_server_t* me, int msec_since_last_period);

int raft_recv_appendentries(raft_server_t* me, int peer, msg_appendentries_t* ae);

int raft_recv_requestvote(raft_server_t* me, int peer, msg_requestvote_t* vr);

int raft_recv_requestvote_response(raft_server_t* me, int peer, msg_requestvote_response_t* r);

void raft_execute_command(raft_server_t* me);

void raft_set_election_timeout(raft_server_t* me, int millisec);

int raft_get_election_timeout(raft_server_t* me);

int raft_vote(raft_server_t* me, int peer);

raft_peer_t* raft_add_peer(raft_server_t* me, int peer_udata);

int raft_remove_peer(raft_server_t* me, int peer);

int raft_get_num_peers(raft_server_t* me);

int raft_recv_command(raft_server_t* me, int peer, msg_command_t* cmd);

int raft_get_log_count(raft_server_t* me);

void raft_set_current_term(raft_server_t* me,int term);

void raft_set_current_index(raft_server_t* me,int idx);

int raft_get_current_term(raft_server_t* me);

void raft_set_current_index(raft_server_t* me,int idx);

int raft_get_current_index(raft_server_t* me);

int raft_is_follower(raft_server_t* me);

int raft_is_leader(raft_server_t* me);

int raft_is_candidate(raft_server_t* me);

int raft_send_requestvote(raft_server_t* me, int peer);

void raft_send_appendentries(raft_server_t* me, int peer);

int raft_append_command(raft_server_t* me_, raft_command_t* c);

void raft_commit_command(raft_server_t* me, int logIndex);

int raft_get_timeout_elapsed(raft_server_t* me);

void raft_set_commit_index(raft_server_t* me, int commit_idx);

void raft_set_last_applied_index(raft_server_t* me, int idx);

void raft_set_state(raft_server_t* me_, int state);

void raft_set_request_timeout(raft_server_t* me_, int millisec);

int raft_get_last_applied_index(raft_server_t* me);

raft_peer_t* raft_peer_new(void* udata);

int raft_peer_is_leader(raft_peer_t* peer);

int raft_peer_get_next_index(raft_peer_t* peer);

void raft_peer_set_next_index(raft_peer_t* peer, int nextIdx);

void raft_set_configuration(raft_server_t* me_, raft_peer_configuration_t* peers);

int raft_votes_is_majority(const int npeers, const int nvotes);
