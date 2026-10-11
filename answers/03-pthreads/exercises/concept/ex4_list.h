// Interface for ex4_list_locks.c: a sorted linked list of ints shared by
// many threads, with three ways of locking it (L04 slides 27-43).
#ifndef EX4_LIST_H
#define EX4_LIST_H

enum list_kind {
    LIST_ONE_MUTEX = 0,   // Solution 1: one mutex for the whole list
    LIST_PER_NODE = 1,    // Solution 2: one mutex in every node (hand-over-hand)
    LIST_RWLOCK = 2,      // Solution 3: one read-write lock for the whole list
};

void list_init(enum list_kind kind);   // start with an empty list; pick the locking style
int list_member(int value);            // 1 if value is in the list, else 0
int list_insert(int value);            // 1 if value was added, 0 if it was already there
int list_delete(int value);            // 1 if value was removed, 0 if it was not there

// Only call these while no other thread is using the list:
int list_to_array(int *out, int max);  // copy the values in list order; returns how many
void list_free(void);                  // free every node

#endif
