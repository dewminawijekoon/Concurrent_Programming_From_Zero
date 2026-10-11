// Interface shared by ex4_bounded_buffer.c and ex5_bounded_buffer_from_memory.c
#ifndef EX4_BOUNDED_BUFFER_H
#define EX4_BOUNDED_BUFFER_H
#define BB_MAX_CAPACITY 1024
int bb_init(int capacity);   // empty buffer with room for `capacity` items; returns 0 on success
void bb_destroy(void);
void bb_put(int item);       // add at the tail; WAITS while the buffer is full
int bb_get(void);            // remove from the head; WAITS while the buffer is empty
#endif
