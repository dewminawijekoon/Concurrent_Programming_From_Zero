// Interface shared by ex1_peterson.c and ex3_peterson_from_memory.c
#ifndef EX1_PETERSON_H
#define EX1_PETERSON_H
void peterson_init(void);
void peterson_lock(int i);    // i is the calling thread's id: 0 or 1
void peterson_unlock(int i);
#endif
