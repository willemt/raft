
typedef void* log_t;

log_t* log_new();

int log_append_entry(log_t* me_, raft_entry_t* c);

int log_count(log_t* me_);

raft_entry_t* log_get_from_idx(log_t* me_, int idx);

raft_entry_t *log_peektail(log_t * me_);
