// Exercise 6 (challenge) - Ricart-Agrawala with real MPI processes   [EXTRA; README section 3.4]
// REQUIRES: mpi
//
// PREDICT FIRST: a process that has finished all its entries cannot simply
//                stop. Why not? (Hint: who would answer the others' requests?)
//
// Every process enters the critical section K times, using Ricart-Agrawala.
// Messages are 2 ints {type, Lamport timestamp}; the MPI tag is the type.
//   REQUEST - "may I enter?"   REPLY - "yes"   DONE - "I need nothing more"
// A process keeps answering messages until it has done its K entries AND has
// received DONE from every other process. Because MPI never lets a later
// message from one sender overtake an earlier one, a DONE always arrives
// after all REQUESTs of that sender.
// Sends use MPI_Isend: a blocking MPI_Send could wait while the partner is
// also sending to us, and then nobody would receive (module 13, section 8.3).
// Inside the critical section call critical_section(my_rank) from cs_log.h.
//
// Test:  make test M=14 T=ex6_ra_mpi
// Run:   make run  M=14 EX=ex6_ra_mpi
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "cs_log.h"

#define K 4
enum { RELEASED, WANTED, HELD };
enum { TAG_REQUEST = 1, TAG_REPLY = 2, TAG_DONE = 3 };

// ---------------------------------------------------------------- given: non-blocking sends
// Every message gets its own buffer and request; all are completed at the end.
static int (*sbuf)[2];
static MPI_Request *sreq;
static int nsent = 0;

static void post(int tag, int ts, int to) {
    sbuf[nsent][0] = tag;
    sbuf[nsent][1] = ts;
    MPI_Isend(sbuf[nsent], 2, MPI_INT, to, tag, MPI_COMM_WORLD, &sreq[nsent]);
    nsent++;
}

static int earlier(int ts_a, int id_a, int ts_b, int id_b) {
    return ts_a < ts_b || (ts_a == ts_b && id_a < id_b);
}

int main(int argc, char **argv) {
    int me, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    int capacity = K * 2 * p + 2 * p + 4;      // more than the messages one process can send
    sbuf = malloc(capacity * sizeof *sbuf);
    sreq = malloc(capacity * sizeof *sreq);
    int *deferred = calloc(p, sizeof *deferred);

    int clock = 0, state = RELEASED, my_ts = 0, replies = 0;
    int entries = 0, done_from = 0;

    // ------------------------------------------------------------ your code
    // TODO: loop while (entries < K || done_from < p - 1):
    //   - RELEASED and entries < K: tick, my_ts = clock, WANTED, replies = 0,
    //     post(TAG_REQUEST, my_ts, j) to every other j. (p == 1: go straight to HELD.)
    //   - HELD: critical_section(me), entries++, RELEASED, REPLY to every deferred
    //     process; after the K-th entry post TAG_DONE to every other process.
    //   - otherwise receive ONE message (MPI_ANY_SOURCE, MPI_ANY_TAG), apply the
    //     Lamport receive rule, then handle REQUEST (defer or reply, as in
    //     exercise 2), REPLY (count; p-1 replies -> HELD) or DONE (done_from++).
    (void)clock; (void)state; (void)my_ts; (void)replies; (void)entries; (void)done_from;
    (void)post; (void)earlier; (void)critical_section;

    // ------------------------------------------------------------ given
    MPI_Waitall(nsent, sreq, MPI_STATUSES_IGNORE);
    free(sbuf); free(sreq); free(deferred);
    MPI_Finalize();
    return 0;
}
