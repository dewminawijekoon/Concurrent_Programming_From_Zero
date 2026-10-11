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
    size_t start = pc->lo;
    for (size_t i = pc->lo; i < pc->hi; i++) {
        if (pc->buf[i] != '\n') continue;
        if (line_has_word(pc->buf + start, i - start)) {
            if (pc->nfound == pc->cap) {   // grow MY buffer; no other thread touches it
                pc->cap = pc->cap ? 2 * pc->cap : 64;
                pc->found = realloc(pc->found, sizeof(long) * (size_t)pc->cap);
            }
            pc->found[pc->nfound++] = pc->nlines;
        }
        pc->nlines++;
        start = i + 1;
    }
    return NULL;
}

long scan_chunk(const char *buf, size_t len, long first_line, int p, long *matches,
                long max_matches, long *nlines) {
    piece *pc = calloc((size_t)p, sizeof *pc);
    pthread_t *t = malloc(sizeof *t * (size_t)p);
    // Cut by bytes with the block rule, then move each cut to a line start.
    // Moving every cut forward keeps them in order, so the pieces never overlap.
    size_t q = len / (size_t)p, rem = len % (size_t)p;
    size_t prev_cut = 0;
    for (int r = 0; r < p; r++) {
        size_t hi = (size_t)(r + 1) * q + ((size_t)(r + 1) < rem ? (size_t)(r + 1) : rem);
        if (r == p - 1) hi = len;
        hi = (hi == 0 || hi >= len) ? hi : after_next_newline(buf, len, hi - 1);
        if (hi < prev_cut) hi = prev_cut;
        pc[r] = (piece){.buf = buf, .lo = prev_cut, .hi = hi};
        prev_cut = hi;
    }
    for (int r = 0; r < p; r++) pthread_create(&t[r], NULL, scan_piece, &pc[r]);
    for (int r = 0; r < p; r++) pthread_join(t[r], NULL);

    // Merge, in thread order, so the line numbers come out sorted.
    long total = 0, base = first_line;
    for (int r = 0; r < p; r++) {
        for (long k = 0; k < pc[r].nfound; k++, total++)
            if (total < max_matches) matches[total] = base + pc[r].found[k];
        base += pc[r].nlines;
        free(pc[r].found);
    }
    if (nlines) *nlines = base - first_line;
    free(pc);
    free(t);
    return total;
}

long scan_file(const char *path, size_t chunk_bytes, int p, long *matches, long max_matches) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    char *buf = malloc(chunk_bytes);   // the ONLY big buffer: memory use stays bounded
    size_t have = 0;                   // bytes in buf (leftover from the last chunk + new)
    long line = 0, total = 0;
    int result_ok = 1;
    for (;;) {
        size_t got = fread(buf + have, 1, chunk_bytes - have, f);
        have += got;
        if (have == 0) break;                         // end of file, nothing left over
        size_t end = have;                            // find the last '\n'
        while (end > 0 && buf[end - 1] != '\n') end--;
        if (end == 0) { result_ok = 0; break; }       // a line longer than the buffer,
                                                      // or a last line without '\n'
        long written = total < max_matches ? total : max_matches;
        long n_lines = 0;
        total += scan_chunk(buf, end, line, p, matches + written, max_matches - written,
                            &n_lines);
        line += n_lines;
        memmove(buf, buf + end, have - end);          // keep the unfinished line
        have -= end;
        if (got == 0 && have > 0) { result_ok = 0; break; }   // file ends without '\n'
    }
    free(buf);
    fclose(f);
    return result_ok ? total : -1;
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
