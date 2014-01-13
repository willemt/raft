
int sender_send(void* caller, void* udata, int peer, int type,
        const unsigned char* data, const int len);

void* sender_new(void* address);

void* sender_poll_msg_data(void* s);

int sender_msgs_available(void* s);


