
typedef void* raft_log_t;

raft_log_t* raft_log_new();

void raft_log_append_entry(raft_log_t* me_, raft_command_t* c);

int raft_log_count(raft_log_t* me_);
