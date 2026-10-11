// Interface for ex1_args_struct.c (the test uses it too).
#ifndef EX1_ARGS_STRUCT_H
#define EX1_ARGS_STRUCT_H

// Everything one thread needs to know, plus room for its answers.
// main() fills the "in" fields, the thread fills the "out" fields.
struct block_arg {
    int id;           // in:  this thread's number, 0 .. nthreads-1
    int nthreads;     // in:  how many threads share the work
    long n;           // in:  the work is the numbers 0, 1, ..., n-1
    long first;       // out: first number this thread handled
    long last;        // out: one past the last number it handled
    long partial;     // out: first + (first+1) + ... + (last-1)
};

void *block_worker(void *arg);
long sum_in_blocks(long n, int nthreads, struct block_arg *args);

#endif
