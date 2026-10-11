// calc/banker.c - deadlock calculator: Banker's algorithm, deadlock detection,
//                 and "find a schedule that deadlocks / completes".
//
// Run (from the repository root):
//   make run M=10 EX=banker ARGS="safe    modules/10-deadlocks/calc/examples/safe.txt"
//   make run M=10 EX=banker ARGS="request modules/10-deadlocks/calc/examples/safe.txt P1 1 0 2"
//   make run M=10 EX=banker ARGS="detect  modules/10-deadlocks/calc/examples/detect.txt"
//   make run M=10 EX=banker ARGS="schedule modules/10-deadlocks/calc/examples/threads.txt"
//   make run M=10 EX=banker ARGS="schedule modules/10-deadlocks/calc/examples/threads.txt at T1=1 T2=1"
// Use "-" instead of a file name to type the input (finish with Ctrl-D).
//
// Input files (lines; '#' starts a comment). See calc/examples/ for samples.
//
//   For safe / request / detect:
//     resources: R1 R2 R3          names of the resource types
//     existing:  10 5 7            total instances (E)      } give one of
//     available: 3 3 2             free instances now (A)   } these two
//     P0: alloc 0 1 0  max 7 5 3   one line per process (safe, request)
//     P0: alloc 0 1 0  need 7 4 3  (or give need instead of max)
//     P0: alloc 0 1 0  request 2 0 0   (detect)
//
//   For schedule:
//     resources: R1=3 R2=2         names AND total instances
//     T1: acquire R1 2; acquire R2 1; release all
//     T2: acquire R2 1; acquire R1 2, R2 1; release R1 2; release R2 2
//   "acquire R1 2, R2 1" asks for both at once: the thread waits until both
//   are free, then takes them in one step. "release all" gives everything back.
//   "at T1=2" means: T1 has finished its first 2 statements.
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXP 10
#define MAXR 8
#define MAXS 12          // statements per thread
#define NAMELEN 16

static int n, m;                         // processes, resource types
static char pname[MAXP][NAMELEN], rname[MAXR][NAMELEN];
static int E[MAXR], A[MAXR];
static int have_E, have_A;
static int Alloc[MAXP][MAXR], Max[MAXP][MAXR], Req[MAXP][MAXR];
static int have_max[MAXP], have_req[MAXP];

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "banker: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(2);
}

static void vec(const int *v) {
    printf("(");
    for (int r = 0; r < m; r++) printf(r ? " %d" : "%d", v[r]);
    printf(")");
}

static void table_header(const char *cols[], int ncols) {
    printf("| %-8s |", "");
    for (int c = 0; c < ncols; c++) {
        printf(" %-*s |", m * 4 > (int)strlen(cols[c]) ? m * 4 : (int)strlen(cols[c]), cols[c]);
    }
    printf("\n|----------|");
    for (int c = 0; c < ncols; c++) {
        int w = m * 4 > (int)strlen(cols[c]) ? m * 4 : (int)strlen(cols[c]);
        for (int i = 0; i < w + 2; i++) putchar('-');
        printf("|");
    }
    printf("\n");
}

static void table_cell(const int *v, const char *col) {
    int w = m * 4 > (int)strlen(col) ? m * 4 : (int)strlen(col);
    char buf[128] = "";
    for (int r = 0; r < m; r++) {
        char x[16];
        snprintf(x, sizeof x, "%s%d", r ? " " : "", v[r]);
        strcat(buf, x);
    }
    printf(" %-*s |", w, buf);
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) *--e = 0;
    return s;
}

static int find_res(const char *s) {
    for (int r = 0; r < m; r++)
        if (strcmp(rname[r], s) == 0) return r;
    die("unknown resource '%s'", s);
    return -1;
}

/* ------------------------------------------------------------ schedule data */
typedef struct {
    int release_all;
    int delta[MAXR];     // + for acquire, - for release
} stmt;
static int nst[MAXP];
static stmt prog[MAXP][MAXS];
static int sched_mode;

static void parse_numbers(char *rest, int *out, const char *what, const char *who) {
    for (int r = 0; r < m; r++) {
        char *end;
        long v = strtol(rest, &end, 10);
        if (end == rest) die("%s: '%s' needs %d numbers", who, what, m);
        if (v < 0) die("%s: negative number in '%s'", who, what);
        out[r] = (int)v;
        rest = end;
    }
}

static void parse_thread(char *name, char *body) {
    if (n == MAXP) die("too many threads (at most %d)", MAXP);
    int p = n++;
    snprintf(pname[p], NAMELEN, "%s", name);
    char *save1;
    for (char *st = strtok_r(body, ";", &save1); st; st = strtok_r(NULL, ";", &save1)) {
        st = trim(st);
        if (!*st) continue;
        if (nst[p] == MAXS) die("%s: too many statements (at most %d)", name, MAXS);
        stmt *s = &prog[p][nst[p]++];
        memset(s, 0, sizeof *s);
        int sign;
        if (strncmp(st, "acquire", 7) == 0) { sign = 1; st += 7; }
        else if (strncmp(st, "request", 7) == 0) { sign = 1; st += 7; }
        else if (strncmp(st, "release", 7) == 0) { sign = -1; st += 7; }
        else die("%s: statement '%s' must start with acquire or release", name, st);
        st = trim(st);
        if (sign < 0 && strcmp(st, "all") == 0) { s->release_all = 1; continue; }
        char *save2;
        for (char *part = strtok_r(st, ",", &save2); part; part = strtok_r(NULL, ",", &save2)) {
            char rn[64];
            int k = 1;
            if (sscanf(part, "%63s %d", rn, &k) < 1) die("%s: cannot read '%s'", name, part);
            if (k <= 0) die("%s: amounts must be positive", name);
            s->delta[find_res(rn)] += sign * k;
        }
    }
}

static void load(const char *path) {
    FILE *f = strcmp(path, "-") == 0 ? stdin : fopen(path, "r");
    if (!f) die("cannot open %s", path);
    char line[512];
    while (fgets(line, sizeof line, f)) {
        char *h = strchr(line, '#');
        if (h) *h = 0;
        char *s = trim(line);
        if (!*s) continue;
        char *colon = strchr(s, ':');
        if (!colon) die("line '%s' has no ':'", s);
        *colon = 0;
        char *key = trim(s), *rest = trim(colon + 1);
        if (strcmp(key, "resources") == 0) {
            m = 0;
            for (char *t = strtok(rest, " \t"); t; t = strtok(NULL, " \t")) {
                if (m == MAXR) die("too many resource types (at most %d)", MAXR);
                char *eq = strchr(t, '=');
                if (eq) { *eq = 0; E[m] = atoi(eq + 1); have_E = 1; sched_mode = 1; }
                snprintf(rname[m++], NAMELEN, "%s", t);
            }
        } else if (strcmp(key, "existing") == 0) {
            parse_numbers(rest, E, "existing", "existing");
            have_E = 1;
        } else if (strcmp(key, "available") == 0) {
            parse_numbers(rest, A, "available", "available");
            have_A = 1;
        } else if (strstr(rest, "acquire") || strstr(rest, "release")) {
            parse_thread(key, rest);
        } else {
            if (m == 0) die("put the 'resources:' line first");
            if (n == MAXP) die("too many processes (at most %d)", MAXP);
            int p = n++;
            snprintf(pname[p], NAMELEN, "%s", key);
            int need[MAXR], have_need = 0;
            char *t = rest;
            while (*(t = trim(t))) {
                char word[32];
                int len = 0;
                while (t[len] && t[len] != ' ' && t[len] != '\t' && len < 31) { word[len] = t[len]; len++; }
                word[len] = 0;
                t += len;
                int *dst;
                if (strcmp(word, "alloc") == 0 || strcmp(word, "has") == 0) dst = Alloc[p];
                else if (strcmp(word, "max") == 0) { dst = Max[p]; have_max[p] = 1; }
                else if (strcmp(word, "need") == 0) { dst = need; have_need = 1; }
                else if (strcmp(word, "request") == 0 || strcmp(word, "req") == 0) { dst = Req[p]; have_req[p] = 1; }
                else { die("%s: unknown word '%s' (use alloc, max, need or request)", key, word); return; }
                // read m numbers, advancing t
                for (int r = 0; r < m; r++) {
                    char *end;
                    long v = strtol(t, &end, 10);
                    if (end == t) die("%s: '%s' needs %d numbers", key, word, m);
                    dst[r] = (int)v;
                    t = end;
                }
            }
            if (have_need && !have_max[p]) {
                for (int r = 0; r < m; r++) Max[p][r] = Alloc[p][r] + need[r];
                have_max[p] = 1;
            }
        }
    }
    if (f != stdin) fclose(f);
    if (m == 0) die("no 'resources:' line");
    if (n == 0) die("no processes or threads");
}

/* --------------------------------------------- available / existing vectors */
static void settle_vectors(void) {
    int used[MAXR] = {0};
    for (int p = 0; p < n; p++)
        for (int r = 0; r < m; r++) used[r] += Alloc[p][r];
    if (!have_A && !have_E) die("give 'existing:' (E) or 'available:' (A)");
    if (have_E && !have_A)
        for (int r = 0; r < m; r++) A[r] = E[r] - used[r];
    if (have_A && !have_E)
        for (int r = 0; r < m; r++) E[r] = A[r] + used[r];
    if (have_A && have_E)
        for (int r = 0; r < m; r++)
            if (E[r] != A[r] + used[r])
                die("existing %s = %d, but allocated %d + available %d = %d", rname[r], E[r],
                    used[r], A[r], used[r] + A[r]);
    for (int r = 0; r < m; r++)
        if (A[r] < 0) die("more %s handed out (%d) than exist (%d)", rname[r], used[r], E[r]);
    printf("Resources:           ");
    for (int r = 0; r < m; r++) printf("%s ", rname[r]);
    printf("\nExisting E:          "); vec(E);
    printf("\nAllocated (sum of C): "); vec(used);
    printf("\nAvailable A = E - allocated: "); vec(A);
    printf("\n\n");
}

/* ------------------------------------------------------------ safety check */
// Runs the Banker's safety check on (avail, Max, alloc) and prints each step.
// Returns 1 if safe.
static int safety(const int *avail, int alloc[MAXP][MAXR], int verbose) {
    int work[MAXR], fin[MAXP] = {0}, seq[MAXP], k = 0;
    memcpy(work, avail, sizeof work);
    if (verbose) { printf("Safety check: work = A = "); vec(work); printf("\n"); }
    for (;;) {
        int found = -1;
        for (int p = 0; p < n && found < 0; p++) {
            if (fin[p]) continue;
            int ok = 1;
            for (int r = 0; r < m; r++)
                if (Max[p][r] - alloc[p][r] > work[r]) ok = 0;
            if (ok) found = p;
        }
        if (found < 0) break;
        int need[MAXR];
        for (int r = 0; r < m; r++) need[r] = Max[found][r] - alloc[found][r];
        for (int r = 0; r < m; r++) work[r] += alloc[found][r];
        fin[found] = 1;
        seq[k++] = found;
        if (verbose) {
            printf("  step %d: %s needs ", k, pname[found]); vec(need);
            printf(" <= work, so it can finish and give back "); vec(alloc[found]);
            printf(" -> work = "); vec(work); printf("\n");
        }
    }
    if (k == n) {
        if (verbose) {
            printf("SAFE. Safe sequence:");
            for (int i = 0; i < n; i++) printf(" %s", pname[seq[i]]);
            printf("\n(Other safe sequences may exist; this one always picks the first\n"
                   " process in the list that fits.)\n");
        }
        return 1;
    }
    if (verbose) {
        printf("  stuck: work = "); vec(work); printf(", and no unfinished process fits:\n");
        for (int p = 0; p < n; p++) {
            if (fin[p]) continue;
            printf("    %s needs ", pname[p]);
            int need[MAXR];
            for (int r = 0; r < m; r++) need[r] = Max[p][r] - alloc[p][r];
            vec(need);
            printf("  (short of");
            int first = 1;
            for (int r = 0; r < m; r++)
                if (need[r] > work[r]) {
                    printf("%s %d %s", first ? "" : ",", need[r] - work[r], rname[r]);
                    first = 0;
                }
            printf(")\n");
        }
        printf("UNSAFE. If every process asked for its full maximum now, these could\n"
               "never all finish. (Unsafe is not yet deadlock: processes might ask for\n"
               "less. The Banker's algorithm simply never enters an unsafe state.)\n");
    }
    return 0;
}

static void print_safe_table(int alloc[MAXP][MAXR]) {
    const char *cols[] = {"Alloc", "Max", "Need = Max - Alloc"};
    table_header(cols, 3);
    for (int p = 0; p < n; p++) {
        int need[MAXR];
        for (int r = 0; r < m; r++) need[r] = Max[p][r] - alloc[p][r];
        printf("| %-8s |", pname[p]);
        table_cell(alloc[p], cols[0]);
        table_cell(Max[p], cols[1]);
        table_cell(need, cols[2]);
        printf("\n");
    }
    printf("\n");
}

static void check_max(void) {
    for (int p = 0; p < n; p++) {
        if (!have_max[p]) die("%s has no 'max' (or 'need')", pname[p]);
        for (int r = 0; r < m; r++) {
            if (Max[p][r] < Alloc[p][r])
                die("%s holds %d %s but its max is %d", pname[p], Alloc[p][r], rname[r], Max[p][r]);
            if (Max[p][r] > E[r])
                die("%s's max for %s (%d) is more than exist (%d)", pname[p], rname[r], Max[p][r], E[r]);
        }
    }
}

static int cmd_safe(void) {
    settle_vectors();
    check_max();
    print_safe_table(Alloc);
    return safety(A, Alloc, 1) ? 0 : 1;
}

static int cmd_request(int argc, char **argv) {
    if (argc < 1 + m) die("usage: request FILE PROCESS n1 ... n%d", m);
    settle_vectors();
    check_max();
    int p = -1;
    for (int i = 0; i < n; i++) if (strcmp(pname[i], argv[0]) == 0) p = i;
    if (p < 0) die("no process called %s", argv[0]);
    int req[MAXR];
    for (int r = 0; r < m; r++) req[r] = atoi(argv[1 + r]);
    print_safe_table(Alloc);
    printf("%s asks for ", pname[p]); vec(req); printf("\n");
    for (int r = 0; r < m; r++)
        if (req[r] > Max[p][r] - Alloc[p][r]) {
            printf("1. Is the request <= Need? NO: it asks for %d %s but declared it needs at most %d more.\n"
                   "=> ERROR: refuse (the process broke its promise).\n", req[r], rname[r], Max[p][r] - Alloc[p][r]);
            return 1;
        }
    printf("1. request <= Need[%s]: yes\n", pname[p]);
    for (int r = 0; r < m; r++)
        if (req[r] > A[r]) {
            printf("2. Is the request <= Available? NO: only %d %s free.\n=> %s must WAIT.\n",
                   A[r], rname[r], pname[p]);
            return 1;
        }
    printf("2. request <= Available: yes\n");
    for (int r = 0; r < m; r++) { A[r] -= req[r]; Alloc[p][r] += req[r]; }
    printf("3. Pretend to grant it: Available = "); vec(A);
    printf(", %s holds ", pname[p]); vec(Alloc[p]); printf("\n4. Is that new state safe?\n\n");
    print_safe_table(Alloc);
    if (safety(A, Alloc, 1)) { printf("\n=> GRANT the request: the new state is safe.\n"); return 0; }
    printf("\n=> %s must WAIT: granting now would lead to an unsafe state.\n", pname[p]);
    return 1;
}

static int cmd_detect(void) {
    settle_vectors();
    for (int p = 0; p < n; p++)
        if (!have_req[p]) die("%s has no 'request' row", pname[p]);
    const char *cols[] = {"C (holds)", "R (requests)"};
    table_header(cols, 2);
    for (int p = 0; p < n; p++) {
        printf("| %-8s |", pname[p]);
        table_cell(Alloc[p], cols[0]);
        table_cell(Req[p], cols[1]);
        printf("\n");
    }
    int work[MAXR], mark[MAXP] = {0}, step = 0;
    memcpy(work, A, sizeof work);
    printf("\nDetection: start with A = "); vec(work); printf("\n");
    for (;;) {
        int found = -1;
        for (int p = 0; p < n && found < 0; p++) {
            if (mark[p]) continue;
            int ok = 1;
            for (int r = 0; r < m; r++) if (Req[p][r] > work[r]) ok = 0;
            if (ok) found = p;
        }
        if (found < 0) break;
        for (int r = 0; r < m; r++) work[r] += Alloc[found][r];
        mark[found] = 1;
        printf("  step %d: %s requests ", ++step, pname[found]); vec(Req[found]);
        printf(" <= A, so it can run to the end and release "); vec(Alloc[found]);
        printf(" -> A = "); vec(work); printf("\n");
    }
    int dead = 0;
    for (int p = 0; p < n; p++) dead += !mark[p];
    if (!dead) { printf("NO DEADLOCK: every process can finish.\n"); return 0; }
    printf("  no unmarked process has its request <= A = "); vec(work); printf("\n");
    printf("DEADLOCK: these processes can never finish:");
    for (int p = 0; p < n; p++) if (!mark[p]) printf(" %s", pname[p]);
    printf("\n");
    return 1;
}

/* ------------------------------------------------------------ schedules */
static int holding_after(int p, int k, int out[MAXR]) {   // holdings after k statements
    int h[MAXR] = {0};
    for (int i = 0; i < k; i++) {
        stmt *s = &prog[p][i];
        if (s->release_all) memset(h, 0, sizeof h);
        else
            for (int r = 0; r < m; r++) {
                h[r] += s->delta[r];
                if (h[r] < 0) die("%s statement %d releases more %s than it holds", pname[p], i + 1, rname[r]);
            }
    }
    memcpy(out, h, sizeof h);
    return 0;
}

static void print_stmt(int p, int i) {
    stmt *s = &prog[p][i];
    if (s->release_all) { printf("release all"); return; }
    int acq = 0;
    for (int r = 0; r < m; r++) if (s->delta[r] > 0) acq = 1;
    printf(acq ? "acquire" : "release");
    int first = 1;
    for (int r = 0; r < m; r++)
        if (s->delta[r]) {
            printf("%s %s %d", first ? "" : ",", rname[r], abs(s->delta[r]));
            first = 0;
        }
}

// state encoding: positions in base (MAXS+1)
typedef struct { int pos[MAXP]; } state;
static long enc(const state *s) {
    long c = 0;
    for (int p = 0; p < n; p++) c = c * (MAXS + 1) + s->pos[p];
    return c;
}
static void avail_of(const state *s, int av[MAXR]) {
    memcpy(av, E, sizeof(int) * MAXR);
    for (int p = 0; p < n; p++) {
        int h[MAXR];
        holding_after(p, s->pos[p], h);
        for (int r = 0; r < m; r++) av[r] -= h[r];
    }
}
static int can_step(const state *s, int p, const int av[MAXR]) {
    if (s->pos[p] >= nst[p]) return 0;
    stmt *st = &prog[p][s->pos[p]];
    if (st->release_all) return 1;
    for (int r = 0; r < m; r++) if (st->delta[r] > av[r]) return 0;
    return 1;
}

#define MAXSTATES 2000000
static long *seen_code;
static int *parent, *by;
static state *states;
static int nstates;

static int lookup(long code) {      // linear probe hash
    static int *table;
    static long cap;
    if (!table) {
        cap = 1 << 22;
        table = malloc(sizeof(int) * (size_t)cap);
        for (long i = 0; i < cap; i++) table[i] = -1;
    }
    long h = (code * 2654435761L) & (cap - 1);
    if (h < 0) h = -h;
    while (table[h] >= 0) {
        if (seen_code[table[h]] == code) return table[h];
        h = (h + 1) & (cap - 1);
    }
    if (nstates >= MAXSTATES) die("too many states");
    table[h] = nstates;
    return -1 - nstates;            // "new, will be stored at index nstates"
}

static void print_path(int idx) {
    int chain[4096], len = 0;
    for (int i = idx; parent[i] >= 0 && len < 4096; i = parent[i]) chain[len++] = i;
    if (len == 0) { printf("    (no steps needed)\n"); return; }
    for (int k = len - 1, step = 1; k >= 0; k--, step++) {
        int i = chain[k], p = by[i], prev = parent[i];
        printf("    %2d. %-4s statement %d: ", step, pname[p], states[prev].pos[p] + 1);
        print_stmt(p, states[prev].pos[p]);
        int av[MAXR];
        avail_of(&states[i], av);
        printf("   -> available ");
        vec(av);
        printf("\n");
    }
}

static void print_alloc_need(const state *s) {
    // still needed = (peak holding over the rest of the program) - holding now
    int alloc[MAXP][MAXR];
    const char *cols[] = {"Allocation", "Still needed", "Max claim"};
    table_header(cols, 3);
    for (int p = 0; p < n; p++) {
        int h[MAXR], peak[MAXR], need[MAXR], maxc[MAXR] = {0};
        holding_after(p, s->pos[p], h);
        memcpy(peak, h, sizeof peak);
        for (int k = s->pos[p]; k <= nst[p]; k++) {
            int x[MAXR];
            holding_after(p, k, x);
            for (int r = 0; r < m; r++) if (x[r] > peak[r]) peak[r] = x[r];
        }
        for (int k = 0; k <= nst[p]; k++) {
            int x[MAXR];
            holding_after(p, k, x);
            for (int r = 0; r < m; r++) if (x[r] > maxc[r]) maxc[r] = x[r];
        }
        for (int r = 0; r < m; r++) { need[r] = peak[r] - h[r]; alloc[p][r] = h[r]; Max[p][r] = h[r] + need[r]; }
        printf("| %-8s |", pname[p]);
        table_cell(h, cols[0]);
        table_cell(need, cols[1]);
        table_cell(maxc, cols[2]);
        printf("\n");
    }
    int av[MAXR];
    avail_of(s, av);
    printf("Available = existing - allocated = "); vec(av); printf("\n");
    printf("(Still needed = the most the thread will hold from here on, minus what it\n"
           " holds now. Max claim = the most it holds at any point of its program.)\n\n");
    safety(av, alloc, 1);
    printf("\n");
}

static int cmd_schedule(int argc, char **argv) {
    if (!sched_mode) die("schedule needs 'resources: R1=3 R2=2' (names with totals)");
    for (int p = 0; p < n; p++) if (nst[p] == 0) die("%s has no statements", pname[p]);
    printf("Resources: ");
    for (int r = 0; r < m; r++) printf("%s=%d ", rname[r], E[r]);
    printf("\n\nPrograms (statement numbers):\n");
    for (int p = 0; p < n; p++) {
        printf("  %s:", pname[p]);
        for (int i = 0; i < nst[p]; i++) { printf("  %d) ", i + 1); print_stmt(p, i); }
        printf("\n");
        int h[MAXR];
        for (int k = 0; k <= nst[p]; k++) holding_after(p, k, h);   // validates releases
        for (int k = 0; k <= nst[p]; k++) {
            holding_after(p, k, h);
            for (int r = 0; r < m; r++)
                if (h[r] > E[r]) die("%s would hold %d %s, but only %d exist", pname[p], h[r], rname[r], E[r]);
        }
    }
    printf("\n");
    state start = {{0}};
    if (argc > 0 && strcmp(argv[0], "at") == 0) {
        for (int i = 1; i < argc; i++) {
            char nm[64];
            int k;
            char *eq = strchr(argv[i], '=');
            if (!eq) die("write positions like T1=2");
            *eq = 0;
            snprintf(nm, sizeof nm, "%s", argv[i]);
            k = atoi(eq + 1);
            int p = -1;
            for (int j = 0; j < n; j++) if (strcmp(pname[j], nm) == 0) p = j;
            if (p < 0) die("no thread called %s", nm);
            if (k < 0 || k > nst[p]) die("%s has only %d statements", nm, nst[p]);
            start.pos[p] = k;
        }
        int av[MAXR];
        avail_of(&start, av);
        for (int r = 0; r < m; r++)
            if (av[r] < 0) die("that position is impossible: more %s held than exist", rname[r]);
        printf("State where");
        for (int p = 0; p < n; p++) printf(" %s has finished %d statement(s)%s", pname[p], start.pos[p], p + 1 < n ? "," : ":");
        printf("\n\n");
    } else {
        printf("State at the start (nobody has run yet):\n\n");
    }
    print_alloc_need(&start);

    // breadth-first search over all interleavings from `start`
    seen_code = malloc(sizeof(long) * MAXSTATES);
    parent = malloc(sizeof(int) * MAXSTATES);
    by = malloc(sizeof(int) * MAXSTATES);
    states = malloc(sizeof(state) * MAXSTATES);
    int r0 = lookup(enc(&start));
    (void)r0;
    states[0] = start; seen_code[0] = enc(&start); parent[0] = -1; by[0] = -1; nstates = 1;
    int dead_idx = -1, done_idx = -1;
    for (int head = 0; head < nstates; head++) {
        state cur = states[head];
        int av[MAXR], moved = 0, alldone = 1;
        avail_of(&cur, av);
        for (int p = 0; p < n; p++) {
            if (cur.pos[p] < nst[p]) alldone = 0;
            if (!can_step(&cur, p, av)) continue;
            moved = 1;
            state nx = cur;
            nx.pos[p]++;
            long c = enc(&nx);
            int f = lookup(c);
            if (f < 0) {
                int idx = -1 - f;
                states[idx] = nx; seen_code[idx] = c; parent[idx] = head; by[idx] = p;
                nstates++;
            }
        }
        if (alldone && done_idx < 0) done_idx = head;
        if (!alldone && !moved && dead_idx < 0) dead_idx = head;
    }
    printf("Explored %d reachable states.\n\n", nstates);
    if (done_idx >= 0) {
        printf("A schedule where every thread COMPLETES:\n");
        print_path(done_idx);
    } else {
        printf("NO schedule lets every thread complete.\n");
    }
    printf("\n");
    if (dead_idx == 0) {
        printf("This state is ALREADY a deadlock.\n");
    } else if (dead_idx > 0) {
        printf("A schedule that DEADLOCKS (shortest):\n");
        print_path(dead_idx);
    }
    if (dead_idx >= 0) {
        state *d = &states[dead_idx];
        int av[MAXR];
        avail_of(d, av);
        printf("    Now nobody can move. Available "); vec(av); printf(":\n");
        for (int p = 0; p < n; p++) {
            int h[MAXR];
            holding_after(p, d->pos[p], h);
            printf("      %-4s holds ", pname[p]); vec(h);
            if (d->pos[p] >= nst[p]) printf(", finished\n");
            else { printf(", waits at statement %d: ", d->pos[p] + 1); print_stmt(p, d->pos[p]); printf("\n"); }
        }
        return 1;
    }
    printf("NO schedule deadlocks: every possible order of steps lets the threads finish.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3 || strcmp(argv[1], "help") == 0) {
        printf("usage:\n"
               "  banker safe     FILE                  Need matrix + safety check, step by step\n"
               "  banker request  FILE P n1 n2 ...      should process P's request be granted?\n"
               "  banker detect   FILE                  deadlock detection with E, A, C, R\n"
               "  banker schedule FILE [at T1=k ...]    allocation / still-needed tables, and\n"
               "                                        schedules that deadlock or complete\n"
               "FILE formats: see the top of modules/10-deadlocks/calc/banker.c and calc/examples/.\n");
        return 2;
    }
    load(argv[2]);
    if (strcmp(argv[1], "safe") == 0) return cmd_safe();
    if (strcmp(argv[1], "request") == 0) return cmd_request(argc - 3, argv + 3);
    if (strcmp(argv[1], "detect") == 0) return cmd_detect();
    if (strcmp(argv[1], "schedule") == 0) return cmd_schedule(argc - 3, argv + 3);
    die("unknown command '%s' (safe, request, detect, schedule)", argv[1]);
    return 2;
}
