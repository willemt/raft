
#ifndef RAFT_LOG_H_
#define RAFT_LOG_H_

typedef void* log_t;

log_t* log_new();

/**
 * Add entry to log.
 * Don't add entry if we've already added this entry (based off ID)
 * Don't add entries with ID=0 
 * @return 0 if unsucessful; 1 otherwise */
int log_append_entry(log_t* me_, raft_entry_t* c);

int log_count(log_t* me_);

/**
 * Delete all logs from this log onwards */
void log_delete(log_t* me_, int idx);

/**
 * Empty the queue. */
void log_empty(log_t * me_);

/**
 * Remove oldest entry
 * @return oldest entry */
void *log_poll(log_t * me_);

raft_entry_t* log_get_from_idx(log_t* me_, int idx);

/*
 * @return youngest entry */
raft_entry_t *log_peektail(log_t * me_);

void log_mark_peer_has_committed(log_t* me_, int idx);

#endif /* RAFT_LOG_H_ */
