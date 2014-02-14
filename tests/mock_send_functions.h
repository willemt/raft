
int sender_send(raft_server_t* raft,
        void* udata,
        int peer,
        raft_message_type_e type,
        const unsigned char* data,
        const int len);

void* sender_new(void* address);

void* sender_poll_msg_data(void* s);

int sender_msgs_available(void* s);


