
int sender_send(void* caller, void* udata, int peer,
        const unsigned char* data, const int len);

void* sender_new();

void* sender_poll_msg(void* s);


