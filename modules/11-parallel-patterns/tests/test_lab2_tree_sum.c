// TIMEOUT: 90
#include <limits.h>

#include "harness.h"

typedef struct tnode {
    long value;
    long size;
    struct tnode *left, *right;
} tnode;

extern atomic_long threads_spawned;
tnode *tree_build(int levels, long *next_value);
void tree_free(tnode *n);
long tree_sum_serial(const tnode *n);
long tree_sum_parallel(const tnode *n, long threshold);

// Nodes whose subtree is bigger than the threshold: only these may fork.
static long big_nodes(const tnode *n, long threshold) {
    if (!n || n->size <= threshold) return 0;
    return 1 + big_nodes(n->left, threshold) + big_nodes(n->right, threshold);
}

static void test_empty_and_tiny(void) {
    CHECK_EQ(tree_sum_parallel(NULL, 10), 0);
    long next = 0;
    tnode *one = tree_build(1, &next);
    CHECK_EQ(tree_sum_parallel(one, 0), 1);
    tree_free(one);
}

static void test_many_sizes_and_thresholds(void) {
    long thresholds[] = {0, 1, 7, 100, 1000, LONG_MAX};
    for (int levels = 1; levels <= 24; levels += (levels < 12 ? 1 : 4)) {
        long next = 0;
        tnode *root = tree_build(levels, &next);
        long n = root->size, want = n * (n + 1) / 2;
        for (int k = 0; k < 6; k++) {
            long th = thresholds[k];
            if (th < 7 && n > 3000) continue;   // keep the number of threads reasonable
            atomic_store(&threads_spawned, 0);
            long got = tree_sum_parallel(root, th);
            long spawned = atomic_load(&threads_spawned);
            CHECK(got == want, "levels=%d (%ld nodes), threshold=%ld: sum %ld, expected %ld",
                  levels, n, th, got, want);
            long limit = 2 * big_nodes(root, th);
            CHECK(spawned <= limit, "levels=%d threshold=%ld: %ld threads started, but only %ld "
                                    "subtrees are bigger than the threshold (forking below the "
                                    "threshold?)", levels, th, spawned, limit / 2);
            if (th == LONG_MAX)
                CHECK(spawned == 0, "threshold bigger than the tree: no thread should start, "
                                    "but %ld did", spawned);
            if (th <= 100 && n > 1000)
                CHECK(spawned > 0, "levels=%d (%ld nodes) threshold=%ld: no thread was started "
                                   "(did you count with atomic_fetch_add(&threads_spawned, 1)?)",
                      levels, n, th);
        }
        tree_free(root);
    }
}

int main(void) {
    harness_watchdog(80);
    harness_hint("Below the threshold call tree_sum_serial. Above it: pthread_create for the "
                 "left subtree, recurse on the right subtree yourself, then pthread_join. Each "
                 "child needs its own struct for its result.");
    RUN_TEST(test_empty_and_tiny);
    RUN_TEST(test_many_sizes_and_thresholds);
    return harness_report();
}
