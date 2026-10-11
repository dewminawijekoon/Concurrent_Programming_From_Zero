// TIMEOUT: 60
#include "harness.h"

long scan_chunk(const char *buf, size_t len, long first_line, int p, long *matches,
                long max_matches, long *nlines);
long scan_file(const char *path, size_t chunk_bytes, int p, long *matches, long max_matches);

#define MAXM 100000

// Make a deterministic "log": lines of random length (some empty), about 1 in
// 8 containing ERROR at a random place, plus near misses like "ERRO R".
static char *make_log(long nlines, unsigned seed, size_t *len_out, long *ref, long *nref) {
    size_t cap = (size_t)nlines * 100 + 16, len = 0;
    char *s = malloc(cap);
    *nref = 0;
    for (long l = 0; l < nlines; l++) {
        seed = seed * 1103515245u + 12345u;
        int n = (int)((seed >> 8) % 70);
        int has = ((seed >> 20) % 8) == 0;
        int pos = has ? (int)((seed >> 4) % (unsigned)(n + 1)) : -1;
        for (int k = 0; k < n; k++) {
            if (k == pos) { memcpy(s + len, "ERROR", 5); len += 5; }
            s[len++] = (k % 11 == 3) ? 'E' : (k % 7 == 2) ? 'R' : (char)('a' + (k * 7 + l) % 26);
        }
        if (pos == n) { memcpy(s + len, "ERROR", 5); len += 5; }
        if (!has && (seed >> 24) % 5 == 0) { memcpy(s + len, "ERRO R", 6); len += 6; }
        s[len++] = '\n';
        if (has) ref[(*nref)++] = l;
    }
    *len_out = len;
    return s;
}

// The reference answer, computed simply and serially.
static long serial_scan(const char *s, size_t len, long *out) {
    long line = 0, n = 0;
    size_t start = 0;
    for (size_t i = 0; i < len; i++)
        if (s[i] == '\n') {
            for (size_t k = start; k + 5 <= i; k++)
                if (memcmp(s + k, "ERROR", 5) == 0) { out[n++] = line; break; }
            line++;
            start = i + 1;
        }
    return n;
}

static long got[MAXM], ref[MAXM];

static int same(const char *what, long n_got, long n_ref, long offset) {
    if (n_got != n_ref) {
        CHECK(0, "%s: found %ld matching lines, expected %ld", what, n_got, n_ref);
        return 0;
    }
    for (long i = 0; i < n_ref; i++)
        if (got[i] != ref[i] + offset) {
            CHECK(0, "%s: match number %ld is line %ld, expected line %ld (wrong number, or "
                     "not in increasing order)", what, i, got[i], ref[i] + offset);
            return 0;
        }
    return 1;
}

static void test_scan_chunk(void) {
    size_t len;
    long nref;
    char *s = make_log(3000, 7, &len, ref, &nref);
    long check[MAXM];
    CHECK_EQ(serial_scan(s, len, check), nref);   // the generator agrees with itself
    int threads[] = {1, 2, 3, 4, 8, 13};
    for (int k = 0; k < 6; k++) {
        long nl = -1;
        for (long i = 0; i < MAXM; i++) got[i] = -1;
        long n = scan_chunk(s, len, 1000, threads[k], got, MAXM, &nl);
        char what[64];
        snprintf(what, sizeof what, "scan_chunk with %d threads", threads[k]);
        if (!same(what, n, nref, 1000)) break;
        CHECK(nl == 3000, "%s: reported %ld lines, expected 3000", what, nl);
    }
    // tiny inputs: more threads than lines, and an empty buffer
    const char *tiny = "ERROR\n\nxERRORx\nERRO\n";
    long n = scan_chunk(tiny, strlen(tiny), 0, 8, got, MAXM, NULL);
    CHECK(n == 2 && got[0] == 0 && got[1] == 2,
          "4 lines \"ERROR\", \"\", \"xERRORx\", \"ERRO\" with 8 threads: expected lines 0 and 2");
    long nl = -1;
    CHECK_EQ(scan_chunk("", 0, 5, 3, got, MAXM, &nl), 0);
    CHECK_EQ(nl, 0);
    // max_matches limits how much is written, but the count is still complete
    got[1] = -7;
    n = scan_chunk(tiny, strlen(tiny), 0, 2, got, 1, NULL);
    CHECK(n == 2 && got[0] == 0 && got[1] == -7,
          "with max_matches = 1: must return 2 but write only matches[0]");
    free(s);
}

static void write_file(const char *path, const char *s, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) { perror("fopen"); exit(1); }
    fwrite(s, 1, len, f);
    fclose(f);
}

static void test_scan_file(void) {
    char path[] = "/tmp/cpfz_scan_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) harness_skip("cannot create a temporary file");
    close(fd);
    size_t len;
    long nref;
    char *s = make_log(20000, 99, &len, ref, &nref);
    write_file(path, s, len);
    size_t chunks[] = {100, 257, 4096, 1 << 20};
    int threads[] = {1, 3, 4};
    for (int c = 0; c < 4; c++)
        for (int k = 0; k < 3; k++) {
            for (long i = 0; i < MAXM; i++) got[i] = -1;
            long n = scan_file(path, chunks[c], threads[k], got, MAXM);
            char what[80];
            snprintf(what, sizeof what, "scan_file chunk=%zu bytes, %d threads", chunks[c],
                     threads[k]);
            if (!same(what, n, nref, 0)) goto done;
        }
    // a line of 80+ bytes cannot fit into a 50-byte buffer
    CHECK(scan_file(path, 50, 2, got, MAXM) == -1,
          "a line longer than chunk_bytes must give -1 (otherwise it would be split)");
    // an empty file has no matches
    write_file(path, "", 0);
    CHECK_EQ(scan_file(path, 64, 2, got, MAXM), 0);
    // no final newline
    write_file(path, "ERROR\nabc", 9);
    CHECK(scan_file(path, 64, 2, got, MAXM) == -1, "a file that does not end with a newline must "
                                                   "give -1");
    CHECK(scan_file("/nonexistent/dir/file.log", 64, 2, got, MAXM) == -1,
          "a missing file must give -1");
done:
    unlink(path);
    free(s);
}

int main(void) {
    harness_watchdog(50);
    harness_hint("Pieces must start at line beginnings, and line numbers must be global: add "
                 "the number of lines in all earlier pieces (and earlier chunks). Keep the "
                 "unfinished last line of a chunk for the next one.");
    RUN_TEST(test_scan_chunk);
    RUN_TEST(test_scan_file);
    return harness_report();
}
