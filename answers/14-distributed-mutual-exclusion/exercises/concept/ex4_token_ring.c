// Exercise 4 (core) - Token ring with threads that only pass messages   [EXTRA; README section 3.3]
//
// PREDICT FIRST: 5 nodes, and only node 3 wants the critical section. How
//                many token passes can it take before node 3 gets in? And if
//                all 5 nodes always want it, in what order do they enter?
//
// Each node is a thread. The threads share NO variables: they talk only
// through mailboxes (given below; think of them as network links). The
// nodes form a ring 0 -> 1 -> ... -> n-1 -> 0, and one token travels around
// it. Only the node holding the token may enter the critical section.
//
// The token carries `remaining`: how many critical-section entries are still
// to be done by ALL nodes together (it starts at n * k). Each node wants k
// entries. When a node holds the token:
//   - if remaining == 0: everyone is done. Pass the token on (so the next
//     node also learns it) and stop - BUT the node that made remaining 0
//     stops without passing it on, so the token's last trip ends there.
//   - else, if I still want entries: enter (call cfg->critical), count it,
//     remaining--;
//   - then pass the token to the next node.
//
// Test:  make test M=14 T=ex4_token_ring
// Run:   make run  M=14 EX=ex4_token_ring
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define RING_MAXN 16

typedef struct { int remaining; long passes; } token_t;

typedef struct {
    int n, k;                                  // nodes, entries wanted by each node
    void (*critical)(int id, void *ctx);       // the critical section
    void *ctx;
} ring_cfg;

// ---------------------------------------------------------------- given: mailboxes
// One slot per node: holds at most one token (there is only one token).
static struct {
    pthread_mutex_t mu;
    pthread_cond_t cv;
    int full;
    token_t tok;
} box[RING_MAXN];

static void mailbox_put(int node, token_t t) {
    pthread_mutex_lock(&box[node].mu);
    box[node].tok = t;
    box[node].full = 1;
    pthread_cond_signal(&box[node].cv);
    pthread_mutex_unlock(&box[node].mu);
}

static token_t mailbox_get(int node) {
    pthread_mutex_lock(&box[node].mu);
    while (!box[node].full) pthread_cond_wait(&box[node].cv, &box[node].mu);
    box[node].full = 0;
    token_t t = box[node].tok;
    pthread_mutex_unlock(&box[node].mu);
    return t;
}

// ---------------------------------------------------------------- your code
void ring_node(int id, const ring_cfg *cfg) {
    int next = (id + 1) % cfg->n;
    int left = cfg->k;                         // entries I still want
    int i_finished_it = 0;                     // did I make remaining 0?
    for (;;) {
        token_t t = mailbox_get(id);           // wait for the token
        if (t.remaining == 0) {
            if (!i_finished_it) mailbox_put(next, t);   // tell the next node
            return;
        }
        if (left > 0) {
            cfg->critical(id, cfg->ctx);       // only the token holder gets here
            left--;
            t.remaining--;
            if (t.remaining == 0) i_finished_it = 1;
        }
        t.passes++;
        mailbox_put(next, t);
    }
}

// ---------------------------------------------------------------- given: start the ring
struct node_arg { int id; const ring_cfg *cfg; };
static void *node_main(void *p) {
    struct node_arg *a = p;
    ring_node(a->id, a->cfg);
    return NULL;
}

void ring_run(const ring_cfg *cfg) {
    pthread_t th[RING_MAXN];
    struct node_arg arg[RING_MAXN];
    for (int i = 0; i < cfg->n; i++) {
        pthread_mutex_init(&box[i].mu, NULL);
        pthread_cond_init(&box[i].cv, NULL);
        box[i].full = 0;
    }
    mailbox_put(0, (token_t){cfg->n * cfg->k, 0});   // node 0 starts with the token
    for (int i = 0; i < cfg->n; i++) {
        arg[i] = (struct node_arg){i, cfg};
        pthread_create(&th[i], NULL, node_main, &arg[i]);
    }
    for (int i = 0; i < cfg->n; i++) pthread_join(th[i], NULL);
}

#ifndef TESTING
static void show(int id, void *ctx) { (void)ctx; printf("node %d is in the critical section\n", id); }
int main(void) {
    ring_cfg cfg = {4, 2, show, NULL};
    ring_run(&cfg);
    return 0;
}
#endif
