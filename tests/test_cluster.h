#ifndef __TEST_CLUSTER__
#define __TEST_CLUSTER__

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "CuTest.h"

#include "raft.h"
#include "raft_log.h"
#include "raft_private.h"

// check code assertions
// #define TEST_CLUSTER_CODE_ASSERTION

// for debug use
// #define TEST_CLUSTER_PRINT_LOG
static void info(int state, const char *info) {
    #ifdef TEST_CLUSTER_PRINT_LOG
    static int last_print_state = 0;
    if (state != last_print_state) {
        printf("\n[STATE %d]\n", state);
        last_print_state = state;
    }
    printf("+ %s\n", info);
    #endif
}

static CuTest *tc;

// data structure size
#define MAX_SERVERS     10
#define MAX_TERM        10
#define MAX_LOG_ENTRIES 20
#define MAX_EACH_MSGS   100
#define MSG_TYPES       5
#define MAX_MSGS        (MAX_EACH_MSGS * MSG_TYPES)
#define MAX_MSG_ENTRIES (MAX_EACH_MSGS * MAX_LOG_ENTRIES)

// fake time out settings
#define ELECTION_TIMEOUT 1000
#define REQUEST_TIMEOUT  200

// server data structure
typedef struct {
    int id;
    raft_server_t* server;
} server_t;

static server_t sv[MAX_SERVERS];
static int num_server = 0;

// msg types
typedef enum {
    MSG_NIL,
    MSG_REQUESTVOTE             = 1,
    MSG_REQUESTVOTE_RESPONSE    = 2,
    MSG_APPENDENTRIES           = 4,
    MSG_APPENDENTRIES_RESPONSE  = 8,
    MSG_SNAPSHOT                = 16
} msg_type_e;

#ifdef TEST_CLUSTER_PRINT_LOG
static const char *get_msg_type_str(msg_type_e type) {
    switch (type)
    {
    case MSG_NIL:
        return "(nil)";
    case MSG_REQUESTVOTE:
        return "RV";
    case MSG_REQUESTVOTE_RESPONSE:
        return "RVR";
    case MSG_APPENDENTRIES:
        return "AE";
    case MSG_APPENDENTRIES_RESPONSE:
        return "AER";
    case MSG_SNAPSHOT:
        return "SS";
    default:
        CuAssertTrue(tc, 0);
    }
    return "";
}
#endif

// client cmd types
typedef enum {
    CMD1,
    CMD2
} cmd_entry_e;

// snapshot msg type
typedef struct {
    raft_term_t term;
    raft_index_t last_included_idx;
    raft_term_t last_included_term;
} msg_snapshot_t;

// msg type
typedef struct {
    msg_type_e type;
    int from;
    int to;
    size_t len;
    void *msg;
} msg_t;

// network msg buf
static msg_t                        msgs[MAX_MSGS + 1];  // index 1 is never used
static msg_entry_t                  msg_entry[MAX_MSG_ENTRIES];
static msg_entry_response_t         msg_entry_response[MAX_EACH_MSGS];
static msg_requestvote_t            msg_requestvote[MAX_EACH_MSGS];
static msg_requestvote_response_t   msg_requestvote_response[MAX_EACH_MSGS];
static msg_appendentries_t          msg_appendentries[MAX_EACH_MSGS];
static msg_appendentries_response_t msg_appendentries_response[MAX_EACH_MSGS];
static msg_snapshot_t               msg_snapshot[MAX_EACH_MSGS];

// next msg pointer
static msg_t                        *next_msg;
static msg_entry_t                  *next_msg_entry;
static msg_entry_response_t         *next_msg_entry_response;
static msg_requestvote_t            *next_msg_requestvote;
static msg_requestvote_response_t   *next_msg_requestvote_response;
static msg_appendentries_t          *next_msg_appendentries;
static msg_appendentries_response_t *next_msg_appendentries_response;
static msg_snapshot_t               *next_msg_snapshot;

// log entry
static raft_entry_data_t entry1 = {.buf = (void*)"1", .len = 1};
static raft_entry_data_t entry2 = {.buf = (void*)"2", .len = 1};

// bit-wise network partition data structure
static int network_partition = 0;

// network partition functions
static void add_to_network_partition(int node_id) {
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network partition add server id: S%d\n", node_id);
    #endif
    network_partition |= 1 << node_id;
}
static void clear_network_partition() {
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network clear partition\n");
    #endif
    network_partition = 0;
}
static void delete_from_network_partition(int node_id) {
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network partition delete server id: S%d\n", node_id);
    #endif
    network_partition &= ~(1 << node_id);
}
static int is_partitioned_node(int node_id) {
    return network_partition & (1 << node_id);
}

static msg_t *set_msg(void *msg, msg_type_e type, int from, int to) {
    static msg_t m;
    m.type = type;
    m.from = from;
    m.to = to;
    m.msg = msg;
    switch (type)
    {
    case MSG_REQUESTVOTE:
        m.len = sizeof(msg_requestvote_t);
        break;
    case MSG_REQUESTVOTE_RESPONSE:
        m.len = sizeof(msg_requestvote_response_t);
        break;
    case MSG_APPENDENTRIES:
        m.len = sizeof(msg_appendentries_t);
        break;
    case MSG_APPENDENTRIES_RESPONSE:
        m.len = sizeof(msg_appendentries_response_t);
        break;
    case MSG_SNAPSHOT:
        m.len = sizeof(msg_snapshot_t);
        break;
    default:
        CuAssertTrue(tc, 0);
    }
    return &m;
}

static long get_seq() {
    return (long)(next_msg - &msgs[1]);
}

// get a copy of the msg to be added so that we can check it simply
static msg_t msg_hook;

static void do_add_msg(msg_t *m) {
    if (!m || !m->msg) {
        CuAssertTrue(tc, 0);
    }
    memcpy(&msg_hook, m, sizeof(*m));
    if (is_partitioned_node(m->from) || is_partitioned_node(m->to)) {
        #ifdef TEST_CLUSTER_PRINT_LOG
        printf(" - Network drop unreachable msg, type: %s, direction S%d -> S%d\n",
            get_msg_type_str(m->type), m->from, m->to);
        #endif
        return;
    }
    memcpy(next_msg++, m, sizeof(*m));
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network add seq: %ld, type: %s, direction S%d -> S%d\n",
        get_seq(), get_msg_type_str(m->type), m->from, m->to);
    #endif
}

// add msg to network
static void add_msg(void* msg, msg_type_e type, raft_server_t* from, raft_node_t* to) {
    int from_id = raft_get_nodeid(from);
    int to_id = raft_node_get_id(to);
    msg_t *m = set_msg(msg, type, from_id, to_id);
    do_add_msg(m);
}

// get the msg
static msg_t *get_msg(int seq) {
    return &msgs[seq];
}

// assert msg matches type
static void assert_msg_seq_type(int seq, msg_type_e type) {
    msg_t *m = get_msg(seq);
    CuAssertTrue(tc, m != NULL);
    if (type != MSG_NIL) {
        assert(m->type & type);
        CuAssertTrue(tc, m->type & type);
    }
}

// assert last msg matches type
static void assert_last_msg_type(msg_type_e type) {
    if (type != MSG_NIL) {
        CuAssertTrue(tc, msg_hook.type & type);
    }
}

// duplicate the msg. seq starts from 1
static void dup_msg(int seq) {
    msg_t *m = get_msg(seq);
    memcpy(next_msg++, m, sizeof(*m));
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network duplicate seq: %d -> %ld, type: %s, direction S%d -> S%d\n",
        seq, get_seq(), get_msg_type_str(m->type), m->from, m->to);
    #endif
}

// delete the msg after delivering
static void delete_msg(int seq) {
    msg_t *m = get_msg(seq);
    CuAssertTrue(tc, m != NULL);
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network delete seq: %d, type: %s, direction S%d -> S%d\n", seq, get_msg_type_str(m->type), m->from, m->to);
    #endif
    memset(m, 0, sizeof(*m));
}

// drop the msg
static void drop_msg(int seq) {
    msg_t *m = get_msg(seq);
    CuAssertTrue(tc, m != NULL);
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network drop seq: %d, type: %s, direction S%d -> S%d\n", seq, get_msg_type_str(m->type), m->from, m->to);
    #endif
    memset(m, 0, sizeof(*m));
}

// callback send functions
static int raft_cbs_send_requestvote(raft_server_t* raft, void* udata, raft_node_t* node, msg_requestvote_t* msg) {
    memcpy(next_msg_requestvote, msg, sizeof(*msg));
    add_msg(next_msg_requestvote++, MSG_REQUESTVOTE, raft, node);
    return 0;
}

static int raft_cbs_send_appendentries(raft_server_t* raft, void* udata, raft_node_t* node, msg_appendentries_t* msg) {
    memcpy(next_msg_appendentries, msg, sizeof(*msg));
    next_msg_appendentries->entries = next_msg_entry;
    for (int i = 0; i < msg->n_entries; i++) {
        memcpy(next_msg_entry++, &(msg->entries[i]), sizeof(*next_msg_entry));
    }
    add_msg(next_msg_appendentries++, MSG_APPENDENTRIES, raft, node);
    return 0;
}

static int raft_cbs_send_snapshot(raft_server_t* raft, void *user_data, raft_node_t* node) {
    next_msg_snapshot->term = raft_get_current_term(raft);
    next_msg_snapshot->last_included_idx = raft_get_snapshot_last_idx(raft);
    next_msg_snapshot->last_included_term = raft_get_snapshot_last_term(raft);
    add_msg(next_msg_snapshot++, MSG_SNAPSHOT, raft, node);
    return 0;
}

// do nothing functions
static int raft_cbs_persist_vote  (raft_server_t* r, void *u, raft_node_id_t v)                { return 0; }
static int raft_cbs_persist_term  (raft_server_t* r, void *u, raft_term_t t, raft_node_id_t v) { return 0; }
static int raft_cbs_applylog      (raft_server_t* r, void *u, raft_entry_t *e, raft_index_t i) { return 0; }
static int raft_cbs_logentry_offer(raft_server_t* r, void *u, raft_entry_t *e, raft_index_t i) { return 0; }
static int raft_cbs_logentry_pop  (raft_server_t* r, void *u, raft_entry_t *e, raft_index_t i) { return 0; }

// log when macro TEST_CLUSTER_PRINT_LOG is defined
static void raft_cbs_log(raft_server_t* raft, raft_node_t* node, void *udata, const char *buf) {
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" * Server id: %d, peer id: %d, info: %s", raft_get_nodeid(raft), (node ? raft_node_get_id(node) : -1), buf);
    size_t len = strlen(buf);
    if (buf[len - 1] != '\n')
        printf("\n");
    #endif
}

// callback functions setting
static raft_cbs_t raft_cbs_funcs = {
    .send_requestvote   = raft_cbs_send_requestvote,
    .send_appendentries = raft_cbs_send_appendentries,
    .send_snapshot      = raft_cbs_send_snapshot,
    .applylog           = raft_cbs_applylog,
    .persist_vote       = raft_cbs_persist_vote,
    .persist_term       = raft_cbs_persist_term,
    .log_offer          = raft_cbs_logentry_offer,
    .log_pop            = raft_cbs_logentry_pop,
    .log                = raft_cbs_log
};

// init data structure functions
static void init_client_entry() {
    for (size_t i = 0; i < MAX_MSG_ENTRIES; i++) {
        msg_entry[i].id = i;
        msg_entry[i].type = RAFT_LOGTYPE_NORMAL;
    }
    next_msg_entry = &msg_entry[0];
}

static void init_network_pointers() {
    next_msg                        = &msgs[1];  // index 1 is never used
    next_msg_entry_response         = &msg_entry_response[0];
    next_msg_requestvote            = &msg_requestvote[0];
    next_msg_requestvote_response   = &msg_requestvote_response[0];
    next_msg_appendentries          = &msg_appendentries[0];
    next_msg_appendentries_response = &msg_appendentries_response[0];
    next_msg_snapshot               = &msg_snapshot[0];

}

static void init_network() {
    init_network_pointers();
    for (int i = 0; i < MAX_MSGS; i++)
        memset(next_msg++,                        0, sizeof(*next_msg));
    for (int i = 0; i < MAX_EACH_MSGS; i++) {
        memset(next_msg_entry_response++,         0, sizeof(*next_msg_entry_response));
        memset(next_msg_requestvote++,            0, sizeof(*next_msg_requestvote));
        memset(next_msg_requestvote_response++,   0, sizeof(*next_msg_requestvote_response));
        memset(next_msg_appendentries++,          0, sizeof(*next_msg_appendentries));
        memset(next_msg_appendentries_response++, 0, sizeof(*next_msg_appendentries_response));
        memset(next_msg_snapshot++,               0, sizeof(*next_msg_snapshot));
    }
    init_network_pointers();
    network_partition = 0;
}

// init cluster
static void rc_init_cluster(int n_server, CuTest *_tc) {
    CuAssertTrue(_tc, n_server <= MAX_SERVERS);

    // free exist servers
    for (int i = 0; i < num_server; i++) {
        raft_free(sv[i].server);
        sv[i].server = NULL;
        sv[i].id = 0;
    }

    tc = _tc;
    num_server = n_server;

    // create servers.
    for (int i = 0; i < num_server; i++) {
        sv[i].id = i;
        sv[i].server = raft_new();
        CuAssertTrue(tc, sv[i].server != NULL);
        raft_set_callbacks(sv[i].server, &raft_cbs_funcs, &sv[i]);
        raft_set_election_timeout(sv[i].server, ELECTION_TIMEOUT);

        // we set rand timeout greater than election timeout
        while (((raft_server_private_t*)sv[i].server)->election_timeout_rand == ELECTION_TIMEOUT)
            raft_set_election_timeout(sv[i].server, ELECTION_TIMEOUT);
        raft_set_request_timeout(sv[i].server, REQUEST_TIMEOUT);
        raft_randomize_election_timeout(sv[i].server);
    }

    // connect servers.
    for (int i = 0; i < num_server; i++) {
        for (int j = 0; j < num_server; j++) {
            int is_self = sv[i].id == sv[j].id;
            raft_node_t* node = raft_add_node(sv[i].server, &sv[j], sv[j].id, is_self);
            CuAssertTrue(tc, node != NULL); 
            raft_node_set_voting_committed(node, true);
            raft_node_set_active(node, true);
        }
    }

    // init client entry
    init_client_entry();

    // init network
    init_network();
}

static void rc_election_timeout(int server_id) {
    CuAssertTrue(tc, !raft_is_leader(sv[server_id].server));
    CuAssertTrue(tc, raft_periodic(sv[server_id].server, ELECTION_TIMEOUT * 2) != RAFT_ERR_SHUTDOWN);
}

static void rc_request_timeout(int server_id) {
    CuAssertTrue(tc, raft_is_leader(sv[server_id].server));
    CuAssertTrue(tc, raft_periodic(sv[server_id].server, REQUEST_TIMEOUT) != RAFT_ERR_SHUTDOWN);
}

// simulate restart
static void rc_restart(int server_id) {
    raft_server_private_t* me = (raft_server_private_t*)sv[server_id].server;
    me->state = RAFT_STATE_FOLLOWER;
    me->commit_idx = me->snapshot_last_idx;
    me->current_leader = NULL;
    me->timeout_elapsed = 0;
    me->last_applied_idx = me->commit_idx;
    raft_randomize_election_timeout((raft_server_t*)me);
}

// deliver msg functions
static void rc_deliver_requestvote(
    raft_server_t* me, raft_node_t* sender, msg_requestvote_t* m, msg_requestvote_response_t *r)
{
    CuAssertTrue(tc, raft_recv_requestvote(me, sender, m, r) != RAFT_ERR_SHUTDOWN);
}
static void rc_deliver_requestvote_response(
    raft_server_t* me, raft_node_t* sender, msg_requestvote_response_t* m)
{
    CuAssertTrue(tc, raft_recv_requestvote_response(me, sender, m) != RAFT_ERR_SHUTDOWN);
}
static void rc_deliver_appendentries(
    raft_server_t* me, raft_node_t* sender, msg_appendentries_t* m, msg_appendentries_response_t *r)
{
    CuAssertTrue(tc, raft_recv_appendentries(me, sender, m, r) != RAFT_ERR_SHUTDOWN);
}
static void rc_deliver_appendentries_response(
    raft_server_t* me, raft_node_t* sender, msg_appendentries_response_t *m)
{
    CuAssertTrue(tc, raft_recv_appendentries_response(me, sender, m) != RAFT_ERR_SHUTDOWN);
}

// deliver snapshot msg
static void rc_deliver_snapshot_request(
    raft_server_t* me, raft_node_t* sender, msg_snapshot_t* m, msg_appendentries_response_t *r)
{
    r->success = false;
    r->current_idx = raft_get_current_idx(me);
    r->first_idx = r->current_idx;
    r->term = raft_get_current_term(me);

    // according to raft paper
    if (m->term < raft_get_current_term(me))
        return;

    int e = raft_begin_load_snapshot(me, m->last_included_term, m->last_included_idx);
    if (e != 0) {
        if (e == RAFT_ERR_SNAPSHOT_ALREADY_LOADED) {
            // to inform the Leader to update next idx and match idx if needed
            goto out;
        }
        else if (e == -1) {
            return;
        }
        CuAssertTrue(tc, 0);  // maybe RAFT_ERR_SHUTDOWN
    }

    // re-add other servers since servers have been removed after load snapshot.
    for (int j = 0, i = raft_get_nodeid(me); j < num_server; j++) {
        int is_self = sv[i].id == sv[j].id;
        raft_node_t* node = raft_add_node(sv[i].server, &sv[j], sv[j].id, is_self);
        if (!node)
            continue;
        raft_node_set_voting_committed(node, true);
        raft_node_set_active(node, true);
    }
    raft_end_load_snapshot(me);

out:
    r->success = true;
    r->current_idx = m->last_included_idx;
    r->first_idx = r->current_idx;
    r->term = raft_get_current_term(me);
}

// deliver a msg by seq
static void rc_deliver(int seq, msg_type_e type) {
    msg_t *m = get_msg(seq);
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" - Network deliver seq: %d, type: %s, direction S%d <- S%d\n", seq, get_msg_type_str(m->type), m->to, m->from);
    #endif
    assert_msg_seq_type(seq, type);
    void *msg = m->msg;
    raft_server_t *me = sv[m->to].server;
    raft_node_t *sender = raft_get_node(me, m->from);
    msg_t *r = NULL;
    switch (m->type)
    {
    case MSG_APPENDENTRIES:
        // ensure message type and len match
        CuAssertTrue(tc, m->len == sizeof(msg_appendentries_t));
        r = set_msg(next_msg_appendentries_response++, MSG_APPENDENTRIES_RESPONSE, m->to, m->from);
        rc_deliver_appendentries(me, sender, (msg_appendentries_t*)msg, (msg_appendentries_response_t*)r->msg);
        break;
    case MSG_APPENDENTRIES_RESPONSE:
        CuAssertTrue(tc, m->len == sizeof(msg_appendentries_response_t));
        rc_deliver_appendentries_response(me, sender, (msg_appendentries_response_t*)msg);
        break;
    case MSG_REQUESTVOTE:
        CuAssertTrue(tc, m->len == sizeof(msg_requestvote_t));
        // raft servers only vote for candidate after election timeout,
        // advance timeout_elapsed to make the server votable.
        if (((raft_server_private_t*)me)->state != RAFT_STATE_LEADER
            && raft_get_timeout_elapsed(me) < ELECTION_TIMEOUT)
        {
            CuAssertTrue(tc, raft_periodic(me, ELECTION_TIMEOUT) != RAFT_ERR_SHUTDOWN);
        }
        r = set_msg(next_msg_requestvote_response++, MSG_REQUESTVOTE_RESPONSE, m->to, m->from);
        rc_deliver_requestvote(me, sender, (msg_requestvote_t*)msg, (msg_requestvote_response_t*)r->msg);
        break;
    case MSG_REQUESTVOTE_RESPONSE:
        CuAssertTrue(tc, m->len == sizeof(msg_requestvote_response_t));
        rc_deliver_requestvote_response(me, sender, (msg_requestvote_response_t*)msg);
        break;
    case MSG_SNAPSHOT:
        CuAssertTrue(tc, m->len == sizeof(msg_snapshot_t));
        r = set_msg(next_msg_appendentries_response++, MSG_APPENDENTRIES_RESPONSE, m->to, m->from);
        rc_deliver_snapshot_request(me, sender, (msg_snapshot_t*)msg, (msg_appendentries_response_t*)r->msg);
        break;
    default:
        CuAssertTrue(tc, 0);
    }
    delete_msg(seq);
    if (r)
        do_add_msg(r);
}

// server do snapshot
static void rc_exec_snapshot(int server_id) {
    CuAssertTrue(tc, raft_begin_snapshot(sv[server_id].server, 0) != RAFT_ERR_SHUTDOWN);
    // nothing to do here
    CuAssertTrue(tc, raft_end_snapshot(sv[server_id].server) != RAFT_ERR_SHUTDOWN);
}

// client append log to leader
static void rc_client_operation(int leader, cmd_entry_e cmd) {
    CuAssertTrue(tc, raft_is_leader(sv[leader].server));
    if (cmd == CMD1)
        next_msg_entry->data = entry1;
    else if (cmd == CMD2)
        next_msg_entry->data = entry2;
    else
        CuAssertTrue(tc, 0);
    int ok = raft_recv_entry(sv[leader].server, next_msg_entry++, next_msg_entry_response++) != RAFT_ERR_SHUTDOWN;
    CuAssertTrue(tc, ok);
}

// inv 1: at most one Leader per term
static void assert_inv1() {
    static int leader_term[MAX_TERM];
    for (int i = 0; i < MAX_TERM; i++) {
        leader_term[i] = -1;
    }
    for (int i = 0; i < num_server; i++) {
        if (raft_is_leader(sv[i].server)) {
            int term = raft_get_current_term(sv[i].server);
            CuAssertTrue(tc, term < MAX_TERM);
            CuAssertTrue(tc, leader_term[term] == -1);
            leader_term[term] = i;
        }
    }
}

// inv 2: committed log replicated majority
static void assert_inv2() {
    for (int i = 0; i < num_server; i++) {
        raft_server_t *me = sv[i].server;
        raft_server_private_t *_me = (raft_server_private_t *)sv[i].server;
        int commit_idx = raft_get_commit_idx(me);
        if (!raft_is_leader(me) || commit_idx == 0)
            continue;
        int n = 0;
        raft_entry_t *entries = log_get_from_idx((log_t *)_me->log, log_get_base((log_t *)_me->log) + 1, &n);
        if (!entries || !n)
            continue;
        int part = raft_get_num_snapshottable_logs(me);
        CuAssertTrue(tc, part <= n);
        int n_replicated = 1;
        for (int j = 0; j < num_server; j++) {
            if (i == j)
                continue;
            raft_server_t *other = sv[j].server;
            raft_server_private_t *_other = (raft_server_private_t *)sv[j].server;
            if (raft_get_current_idx(other) < commit_idx || raft_get_snapshot_last_idx(other) >= commit_idx) {
                #ifdef TEST_CLUSTER_PRINT_LOG
                printf(" ? INV2: committed log (idx: %d) is not replicated to: S%d\n", commit_idx, j);
                #endif
                continue;
            }
            int n2 = 0;
            raft_entry_t *other_entries = log_get_from_idx((log_t*)_other->log, log_get_base((log_t *)_me->log) + 1, &n2);
            if (!entries || !n2) {
                #ifdef TEST_CLUSTER_PRINT_LOG
                printf(" ? INV2: committed log (idx: %d) is not replicated to: S%d\n", commit_idx, j);
                #endif
                continue;
            }
            int other_part = commit_idx - log_get_base((log_t *)_me->log);
            CuAssertTrue(tc, other_part <= n2);
            int min_part = (part < other_part) ? part : other_part;
            int ok = 1;
            for (int k = 0; k < min_part; k++) {
                int a_idx = part - 1 - k, b_idx = other_part - 1 - k;
                ok &= ((char*)(entries[a_idx].data.buf))[0] == ((char*)(other_entries[b_idx].data.buf))[0];
                ok &= entries[a_idx].id == other_entries[b_idx].id;
                ok &= entries[a_idx].term == other_entries[b_idx].term;
            }
            if (ok) {
                #ifdef TEST_CLUSTER_PRINT_LOG
                printf(" ? INV2: committed log (idx: %d) replicated to: S%d\n", commit_idx, j);
                #endif
                n_replicated++;
            }
            #ifdef TEST_CLUSTER_PRINT_LOG
            else {
                printf(" ? INV2: committed log (idx: %d) is not replicated to: S%d\n", commit_idx, j);
            }
            #endif
        }
        CuAssertTrue(tc, n_replicated * 2 > num_server);
    }
}

// inv 3: committed log is durable (won't be rolled back)
// It is violated in TLA+, however it won't be violated in c code
// since raft_server.c detects "AE prev conflicts with committed entry"
// and return RAFT_ERR_SHUTDOWN which cause another assertion
static struct {
    int base;
    int snapshottable;
    int commit_idx;
    raft_entry_t entries[MAX_LOG_ENTRIES];
} inv3_entries[MAX_SERVERS];
static void assert_inv3_before_action() {
    for (int i = 0; i < num_server; i++) {
        raft_server_t *me = sv[i].server;
        raft_server_private_t *_me = (raft_server_private_t *)sv[i].server;
        int n = 0;
        int base = log_get_base((log_t *)_me->log);
        inv3_entries[i].base = base;
        inv3_entries[i].snapshottable = raft_get_num_snapshottable_logs(me);
        inv3_entries[i].commit_idx = raft_get_commit_idx(me);
        raft_entry_t *entries = log_get_from_idx((log_t *)_me->log, base + 1, &n);
        if (!entries || !n) {
            continue;
        }
        for (int k = 0; k < n; k++) {
            memcpy(&(inv3_entries[i].entries[k]), &entries[k], sizeof(raft_entry_t));
        }
    }
}
static void assert_inv3_after_action() {
    for (int i = 0; i < num_server; i++) {
        raft_server_t *me = sv[i].server;
        raft_server_private_t *_me = (raft_server_private_t *)sv[i].server;
        int n = 0;
        int base = log_get_base((log_t *)_me->log);
        raft_entry_t *entries = log_get_from_idx((log_t *)_me->log, base + 1, &n);
        if (!entries || !n) {
            continue;
        }
        int commit_idx = raft_get_commit_idx(me);
        int snapshottable = raft_get_num_snapshottable_logs(me);
        int idx = (commit_idx < inv3_entries[i].commit_idx) ? commit_idx : inv3_entries[i].commit_idx;
        int len = (snapshottable < inv3_entries[i].snapshottable) ? snapshottable: inv3_entries[i].snapshottable;
        int ok = 1;
        for (int k = 0; k < len; k++) {
            int a_idx = idx - base - 1 - k, b_idx = idx - inv3_entries[i].base - 1 - k;
            ok &= ((char*)(entries[a_idx].data.buf))[0] == ((char*)(inv3_entries[i].entries[b_idx].data.buf))[0];
            // ok &= entries[a_idx].id == inv3_entries[i].entries[b_idx].id;
            ok &= entries[a_idx].term == inv3_entries[i].entries[b_idx].term;
        }
        CuAssertTrue(tc, ok);
    }
}

// inv 4: Follower's log is less than or equal to leader's log after receiving AE and changing its log
static void assert_inv4(int seq) {
    msg_t *m = get_msg(seq);
    CuAssertTrue(tc, m != NULL);
    CuAssertTrue(tc, m->type == MSG_APPENDENTRIES);
    raft_server_t *me = sv[m->to].server;
    raft_server_t *leader = sv[m->from].server;
    int follower_log_len_before = raft_get_current_idx(me);
    int server_term = ((msg_appendentries_t*)m->msg)->term;
    rc_deliver(seq, MSG_APPENDENTRIES);
    int follower_log_len_after = raft_get_current_idx(me);
    int leader_log_len = raft_get_current_idx(leader);
    #ifdef TEST_CLUSTER_PRINT_LOG
    printf(" ? INV4: follower log length: before: %d after: %d, leader log length: %d\n",
        follower_log_len_before, follower_log_len_after, leader_log_len);
    #endif
    if (follower_log_len_before != follower_log_len_after && server_term == raft_get_current_term(leader)) {
        CuAssertTrue(tc, follower_log_len_after <= leader_log_len);
    }
}

// inv 5: monotonic current term
static int inv5_terms[MAX_SERVERS];
static void assert_inv5_before_action() {
    for (int i = 0; i < num_server; i++)
        inv5_terms[i] = raft_get_current_term(sv[i].server);
}
static void assert_inv5_after_action() {
    int inv5_terms_after[MAX_SERVERS];
    for (int i = 0; i < num_server; i++)
        inv5_terms_after[i] = raft_get_current_term(sv[i].server);
    // assert(0);
    for (int i = 0; i < num_server; i++) {
        #ifdef TEST_CLUSTER_PRINT_LOG
        printf(" ? INV5: for S%d: current term before: %d, current term after: %d\n",
            i, inv5_terms[i], inv5_terms_after[i]);
        #endif
        CuAssertTrue(tc, inv5_terms[i] <= inv5_terms_after[i]);
    }
}

// Did not found these invariants would be violated
// inv 6: monotonic commit index
// inv 7: monotonic match index
// inv 8: (internal)
// inv 9: snapshot last index is less than or equal to commit index
// inv 10: next index is greater than zero

// inv 11: next index is greater than match index
// (it seems that this bug does not cause any disasters)
static void assert_inv11() {
    for (int i = 0; i < num_server; i++) {
        raft_server_t *me = sv[i].server;
        if (!raft_is_leader(me))
            continue;
        for (int j = 0; j < num_server; j++) {
            raft_node_t *node = raft_get_node_from_idx(me, j);
            #ifdef TEST_CLUSTER_PRINT_LOG
            printf(" ? INV11: for S%d: next idx: %ld, match idx: %ld\n",
                j, raft_node_get_next_idx(node), raft_node_get_match_idx(node));
            #endif
            CuAssertTrue(tc, raft_node_get_match_idx(node) < raft_node_get_next_idx(node));
        }
    }
}

// inv 12: AE msg should contain at least one entry
static void assert_inv12(int server_id) {
    clear_network_partition();
    rc_request_timeout(server_id);
    raft_server_t *me = sv[server_id].server;
    for (int i = num_server - 1; i > 0; i--) {
        msg_t *m = next_msg - i;
        CuAssertTrue(tc, m != NULL);
        if (m->type == MSG_SNAPSHOT)
            continue;
        CuAssertTrue(tc, m->type == MSG_APPENDENTRIES);
        int next_idx = raft_node_get_next_idx(raft_get_node(me, (raft_node_id_t)m->to));
        int num_entries_should_send = raft_get_current_idx(me) + 1 - next_idx;
        int num_entries_ae = ((msg_appendentries_t *)m->msg)->n_entries;
        #ifdef TEST_CLUSTER_PRINT_LOG
        printf(" ? INV12: for S%d: should send: %d, really sent: %d\n",
            m->to, num_entries_should_send, num_entries_ae);
        #endif
        CuAssertTrue(tc, num_entries_ae == num_entries_should_send);
    }
}

// inv 13: will exist a Leader
static void assert_inv13() {
    int num_leader = 0;
    for (int i = 0; i < num_server; i++) {
        #ifdef TEST_CLUSTER_PRINT_LOG
        printf(" ? INV13: for S%d: is leader: %d\n", i, raft_is_leader(sv[i].server));
        #endif
        if (raft_is_leader(sv[i].server))
            num_leader++;
    }
    CuAssertTrue(tc, num_leader != 0);
}

#endif //__TEST_CLUSTER__

