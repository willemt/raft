# How does it work?

```c

char* obj = strdup("test");

void *q = llqueue_new();

llqueue_offer(q, obj);

printf("object from queue: %s\n", llqueue_poll(q));

```

# Building
$make

# Todo

- Make lockfree variant using CAS

