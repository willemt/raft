
void* sender_new(void* address);

void* sender_poll_msg_data(void* s);

int sender_msgs_available(void* s);

int sender_requestvote(raft_server_t* raft,
        void* udata, int peer, msg_requestvote_t* msg);

int sender_requestvote_response(raft_server_t* raft,
        void* udata, int peer, msg_requestvote_response_t* msg);

int sender_appendentries(raft_server_t* raft,
        void* udata, int peer, msg_appendentries_t* msg);

int sender_appendentries_response(raft_server_t* raft,
        void* udata, int peer, msg_appendentries_response_t* msg);

int sender_entries(raft_server_t* raft,
        void* udata, int peer, msg_entry_t* msg);

int sender_entries_response(raft_server_t* raft,
        void* udata, int peer, msg_entry_response_t* msg);
