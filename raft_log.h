
typedef void* raft_log_t;

raft_log_t* raft_log_new();

int raft_log_append_entry(raft_log_t* me_, raft_entry_t* c);

int raft_log_count(raft_log_t* me_);

raft_entry_t* raft_log_get_from_idx(raft_log_t* me_, int idx);
