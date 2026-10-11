// TIMEOUT: 20
#include "harness.h"

int global_index(int block_idx, int block_dim, int thread_idx);
int blocks_needed(int n, int threads_per_block);
int simulate_1d_launch(int n, int blocks, int threads, int *owner_block, int *owner_thread);
int grid_stride_count(int n, int total_threads, int g);
void matmul_cell(int bx, int by, int tx, int block_dim_x, int *row, int *col);

static void test_global_index(void) {
    CHECK_EQ(global_index(0, 4, 0), 0);
    CHECK_EQ(global_index(0, 4, 3), 3);
    CHECK_EQ(global_index(2, 4, 1), 9);      // the PREDICT FIRST question
    CHECK_EQ(global_index(3, 256, 17), 785);
}

static void test_blocks_needed(void) {
    CHECK_EQ(blocks_needed(1000, 256), 4);   // 3 blocks would miss elements 768..999
    CHECK_EQ(blocks_needed(1024, 256), 4);   // exact fit: no extra block
    CHECK_EQ(blocks_needed(1025, 256), 5);
    CHECK_EQ(blocks_needed(1, 256), 1);
    CHECK_EQ(blocks_needed(10, 4), 3);
}

static void check_launch(int n, int blocks, int threads, int want_idle) {
    static int ob[5000], ot[5000];
    for (int e = 0; e < n; e++) ob[e] = ot[e] = -1;
    int idle = simulate_1d_launch(n, blocks, threads, ob, ot);
    CHECK(idle == want_idle, "<<<%d, %d>>> on %d elements: %d idle threads, expected %d",
          blocks, threads, n, idle, want_idle);
    int missing = 0, wrong = 0;
    for (int e = 0; e < n; e++) {
        if (ob[e] < 0) { missing++; continue; }
        if (ob[e] * threads + ot[e] != e || ot[e] >= threads) wrong++;
    }
    CHECK(missing == 0, "<<<%d, %d>>> on %d elements: %d elements were handled by nobody", blocks, threads, n, missing);
    CHECK(wrong == 0, "<<<%d, %d>>> on %d elements: %d elements recorded with the wrong (block, thread)", blocks, threads, n, wrong);
}

static void test_simulate(void) {
    check_launch(10, 3, 4, 2);
    check_launch(1000, 4, 256, 24);
    check_launch(1024, 4, 256, 0);
    check_launch(1, 1, 32, 31);
    static int ob[16], ot[16];
    for (int e = 0; e < 10; e++) ob[e] = ot[e] = -1;
    simulate_1d_launch(10, 3, 4, ob, ot);
    CHECK(ob[9] == 2 && ot[9] == 1, "element 9 must be handled by block 2, thread 1 (got block %d, thread %d)", ob[9], ot[9]);
}

static void test_grid_stride(void) {
    // 8 threads in total looping over 20 elements: ids 0..3 get 3 elements, 4..7 get 2.
    CHECK_EQ(grid_stride_count(20, 8, 0), 3);
    CHECK_EQ(grid_stride_count(20, 8, 3), 3);
    CHECK_EQ(grid_stride_count(20, 8, 4), 2);
    CHECK_EQ(grid_stride_count(20, 8, 7), 2);
    CHECK_EQ(grid_stride_count(5, 8, 6), 0);
    long total = 0;
    for (int g = 0; g < 96; g++) total += grid_stride_count(1000, 96, g);
    CHECK(total == 1000, "all threads together must handle every element exactly once (got %ld of 1000)", total);
}

static void test_matmul_cell(void) {
    int r = -1, c = -1;
    matmul_cell(0, 0, 0, 32, &r, &c);
    CHECK(r == 0 && c == 0, "block (0,0) thread 0 -> row %d col %d, expected row 0 col 0", r, c);
    matmul_cell(2, 5, 7, 32, &r, &c);
    CHECK(r == 5 && c == 71, "block (2,5) thread 7, 32 per block -> row %d col %d, expected row 5 col 71", r, c);
    matmul_cell(31, 1023, 31, 32, &r, &c);
    CHECK(r == 1023 && c == 1023, "last thread of the 1024x1024 launch -> row %d col %d, expected 1023, 1023", r, c);
}

int main(void) {
    harness_watchdog(10);
    harness_hint("index = blockIdx * blockDim + threadIdx; round the number of blocks UP: (n + t - 1) / t");
    RUN_TEST(test_global_index);
    RUN_TEST(test_blocks_needed);
    RUN_TEST(test_simulate);
    RUN_TEST(test_grid_stride);
    RUN_TEST(test_matmul_cell);
    return harness_report();
}
