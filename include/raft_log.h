
#ifndef RAFT_LOG_H_
#define RAFT_LOG_H_

typedef void* log_t;

log_t* log_new();

int log_append_entry(log_t* me_, raft_entry_t* c);

int log_count(log_t* me_);

raft_entry_t* log_get_from_idx(log_t* me_, int idx);

raft_entry_t *log_peektail(log_t * me_);

void log_mark_peer_has_committed(log_t* me_, int idx);

#endif /* RAFT_LOG_H_ */
