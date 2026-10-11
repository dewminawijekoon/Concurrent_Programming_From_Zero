// Exercise 3 (challenge) - a file bigger than memory, per-thread buffers
//                                          [L10 slide 43 (block partitioning); design]
//
// PREDICT FIRST: you cut a buffer of text into 4 equal pieces by BYTES and
//                give one piece to each thread. Why can a line be counted
//                twice, or a word like ERROR be missed? How do you fix it?
//
// The job: find every line that contains the word ERROR in a log file, and
// list their line numbers (0 = first line) in increasing order.
// Two design rules from exam questions:
//   1. The file may be bigger than memory. So read it in CHUNKS of at most
//      chunk_bytes, process one chunk, then reuse the same buffer.
//   2. Threads must not fight over one shared output list (a lock on it
//      makes them wait for each other). Each thread writes its matches into
//      its OWN buffer, and we merge the buffers at the end, in thread order.
//
// Part A - scan_chunk(): buf holds whole lines only (every line ends with
//   '\n'). Split it into p pieces that start at line beginnings (cut by
//   bytes with the ex1 block rule, then move each cut forward to just after
//   the next '\n'). Thread t counts the lines in its piece and stores the
//   LOCAL line numbers of matching lines in its own buffer. After joining,
//   turn local numbers into global ones: thread t's first line is
//   first_line + (lines in the pieces of threads 0 .. t-1).
//   Return the number of matches. Write at most max_matches of them into
//   matches[]. If nlines is not NULL, store the number of lines in buf there.
//
// Part B - scan_file(): read the file in chunks with ONE buffer of
//   chunk_bytes. A chunk usually ends in the middle of a line: process only
//   up to the last '\n', move the leftover bytes to the front of the buffer,
//   and read more after them. Return the number of matches, or -1 if the file
//   cannot be opened, if a line does not fit in the buffer, or if the file
//   does not end with '\n'.
//
// Test:  make test M=11 T=ex3_chunked_scan
// Run:   make run  M=11 EX=ex3_chunked_scan ARGS="/var/log/syslog 65536 4"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORD "ERROR"

// Given: does the line s[0 .. len-1] contain WORD?
static int line_has_word(const char *s, size_t len) {
    size_t w = strlen(WORD);
    for (size_t i = 0; i + w <= len; i++)
        if (memcmp(s + i, WORD, w) == 0) return 1;
    return 0;
}

// Given: the index just after the next '\n' at or after pos (or len if none).
static size_t after_next_newline(const char *buf, size_t len, size_t pos) {
    while (pos < len && buf[pos] != '\n') pos++;
    return pos < len ? pos + 1 : len;
}

// One thread's piece and its private results.
typedef struct {
    const char *buf;
    size_t lo, hi;       // the piece is buf[lo .. hi-1], whole lines only
    long nlines;         // lines in the piece
    long *found;         // local line numbers of matching lines (own buffer)
    long nfound, cap;
} piece;

static void *scan_piece(void *arg) {
    piece *pc = arg;
    // TODO: walk buf[pc->lo .. pc->hi-1]. At every '\n' a line ends:
    //       if line_has_word(start of line, its length), append pc->nlines to
    //       pc->found (grow it with realloc when full); then pc->nlines++.
    (void)pc; (void)line_has_word;
    return NULL;
}

long scan_chunk(const char *buf, size_t len, long first_line, int p, long *matches,
                long max_matches, long *nlines) {
    // TODO 1: make p pieces (calloc) and cut buf into whole-line pieces:
    //         cut by bytes with the block rule, then move each cut with
    //         after_next_newline(buf, len, cut - 1). The last piece ends at len.
    // TODO 2: one thread per piece running scan_piece; join them all.
    // TODO 3: merge in thread order, adding each piece's starting line
    //         number; write at most max_matches; free the piece buffers.
    (void)buf; (void)len; (void)first_line; (void)p; (void)matches; (void)max_matches;
    (void)scan_piece; (void)after_next_newline;
    if (nlines) *nlines = 0;
    return 0;
}

long scan_file(const char *path, size_t chunk_bytes, int p, long *matches, long max_matches) {
    // TODO: fopen; ONE malloc of chunk_bytes; loop: fread after the leftover
    //       bytes, find the last '\n', scan_chunk up to it, add its line count
    //       to the running line number, memmove the leftover to the front.
    //       Return -1 for the error cases listed at the top.
    (void)path; (void)chunk_bytes; (void)p; (void)matches; (void)max_matches;
    return 0;
}

#ifndef TESTING
int main(int argc, char **argv) {
    if (argc < 2) {
        printf("usage: ex3_chunked_scan FILE [CHUNK_BYTES] [THREADS]\n");
        return 2;
    }
    size_t chunk = argc > 2 ? (size_t)atol(argv[2]) : 65536;
    int p = argc > 3 ? atoi(argv[3]) : 4;
    long max = 20, m[20];
    long n = scan_file(argv[1], chunk, p, m, max);
    if (n < 0) {
        printf("could not scan %s (missing file, line longer than %zu bytes, or no final "
               "newline)\n", argv[1], chunk);
        return 1;
    }
    printf("%ld lines contain \"%s\". First ones (0 = first line):", n, WORD);
    for (long i = 0; i < n && i < max; i++) printf(" %ld", m[i]);
    printf("\n");
    return 0;
}
#endif
