// TIMEOUT: 60
// Tests for lab track 3 (Lab 04 Ex 3): the barbershop rules, checked on a log.
#include <string.h>

#include "classic_test_util.h"

void shop_init(int m);
void shop_destroy(void);
int customer_visit(int id);
void barber_work(void);
void shop_close(void);

static long cut_us;
void shop_event(int customer, const char *what, int value) { trace_event(customer, what, value); }
void shop_haircut(int customer) {
    trace_event(-1, "cut_start", customer);
    if (cut_us) sleep_us(cut_us);
    else harness_jitter();
    trace_event(-1, "cut_end", customer);
    atomic_fetch_add(&progress, 1);
}

#define MAXC 200
static int returned[MAXC];
static int n_customers, arrival_max_us;
static atomic_int barber_finished;

static void customer(int id, void *ctx) {
    (void)ctx;
    if (arrival_max_us) sleep_us(harness_rand() % (unsigned)arrival_max_us);
    returned[id] = customer_visit(id);
    atomic_fetch_add(&progress, 1);
}
static void *barber_thread(void *arg) {
    (void)arg;
    barber_work();
    atomic_store(&barber_finished, 1);
    return NULL;
}
static int trickle_gap_ms;
static void trickle(int id, void *ctx) {   // customer id arrives at id * gap
    (void)ctx;
    sleep_ms(id * trickle_gap_ms);
    returned[id] = customer_visit(id);
    atomic_fetch_add(&progress, 1);
}

// Run one day at the shop and check every rule on the event log.
// Returns the number of customers turned away.
static int day(int chairs, int customers, int arrival_us, long haircut_us, harness_worker_fn who) {
    n_customers = customers;
    arrival_max_us = arrival_us;
    cut_us = haircut_us;
    memset(returned, -1, sizeof returned);
    trace_reset();
    atomic_store(&barber_finished, 0);
    shop_init(chairs);
    progress_watch_start(5, "the shop is stuck (deadlock or lost wake-up?)", NULL);
    pthread_t b;
    pthread_create(&b, NULL, barber_thread, NULL);
    harness_parallel(customers, who, NULL);
    shop_close();
    int home = wait_for_flag(&barber_finished, 3000);
    progress_watch_stop();
    CHECK(home, "every customer had left and shop_close() was called, but barber_work() did not return");
    if (!home) return -1;
    pthread_join(b, NULL);
    shop_destroy();

    // ---- check the log ----
    int sits[MAXC] = {0}, balks[MAXC] = {0}, dones[MAXC] = {0}, cuts[MAXC] = {0};
    long t_sit[MAXC], t_cut[MAXC], t_end[MAXC], t_done[MAXC];
    int in_cut = -1, bad = 0;
    for (long i = 0; i < trace_len && bad < 5; i++) {
        trace_entry *e = &trace_log[i];
        const char *a = e->action;
        int c = strcmp(a, "cut_start") == 0 || strcmp(a, "cut_end") == 0 ? (int)e->value : e->thread;
        if (c < 0 || c >= customers) {
            printf("    log entry '%s' names customer %d, who does not exist\n", a, c);
            bad++;
            continue;
        }
        if (strcmp(a, "sit") == 0) {
            sits[c]++;
            t_sit[c] = i;
            if (e->value < 1 || e->value > chairs) {
                printf("    customer %d sat down and then %ld customers were waiting, but there are only %d chairs\n",
                       c, e->value, chairs);
                bad++;
            }
        } else if (strcmp(a, "balk") == 0) {
            balks[c]++;
        } else if (strcmp(a, "done") == 0) {
            dones[c]++;
            t_done[c] = i;
        } else if (strcmp(a, "cut_start") == 0) {
            if (in_cut >= 0) {
                printf("    the barber started cutting customer %d while still cutting customer %d\n", c, in_cut);
                bad++;
            }
            in_cut = c;
            cuts[c]++;
            t_cut[c] = i;
        } else if (strcmp(a, "cut_end") == 0) {
            in_cut = -1;
            t_end[c] = i;
        }
    }
    int served = 0, away = 0;
    for (int c = 0; c < customers && bad < 5; c++) {
        if (returned[c] == 1) {
            served++;
            if (sits[c] != 1 || dones[c] != 1 || cuts[c] != 1 || balks[c] != 0) {
                printf("    customer %d got a haircut (returned 1) but logged sit=%d done=%d balk=%d, "
                       "and the barber cut it %d times (expected 1, 1, 0 and 1)\n",
                       c, sits[c], dones[c], balks[c], cuts[c]);
                bad++;
            } else if (!(t_sit[c] < t_cut[c] && t_cut[c] < t_end[c] && t_end[c] < t_done[c])) {
                printf("    customer %d: the order should be sit -> haircut starts -> haircut ends -> done, but it was (log positions) sit %ld, start %ld, end %ld, done %ld\n", c, t_sit[c], t_cut[c], t_end[c], t_done[c]);
                bad++;
            }
        } else if (returned[c] == 0) {
            away++;
            if (balks[c] != 1 || sits[c] != 0 || cuts[c] != 0) {
                printf("    customer %d left without a haircut (returned 0) but logged balk=%d sit=%d, cut %d times\n",
                       c, balks[c], sits[c], cuts[c]);
                bad++;
            }
        } else {
            printf("    customer %d: customer_visit() returned %d (expected 0 or 1)\n", c, returned[c]);
            bad++;
        }
    }
    CHECK(bad == 0, "%d chairs, %d customers: the shop broke a rule (see above)", chairs, customers);
    CHECK(served + away == customers, "served %d + turned away %d should be %d", served, away, customers);
    if (bad) trace_dump_tail(15);
    return away;
}

// Everybody arrives at once, there is 1 chair and haircuts are slow:
// most customers must be turned away, and nobody may sit on a 2nd chair.
static void test_burst_with_one_chair(void) {
    int away = day(1, 8, 0, 20000, customer);
    CHECK(away >= 1, "8 customers arrived at the same moment with only 1 waiting chair, "
                     "but nobody was turned away");
}

// Customers arrive one by one, slowly, and haircuts are quick: nobody
// should be turned away.
static void test_slow_trickle_nobody_turned_away(void) {
    trickle_gap_ms = 25;
    int away = day(2, 6, 0, 2000, trickle);
    CHECK(away == 0, "customers arrived 25 ms apart and a haircut takes 2 ms, but %d were turned away", away);
}

// Lab Q3.2: customers arrive very fast, many days in a row.
static void test_fast_arrivals_many_days(void) {
    long days = harness_iters(40);
    for (long d = 0; d < days; d++)
        if (day(2, 30, 0, 0, customer) < 0) return;
    printf("  (%ld days with 30 customers arriving at once: no rule broken - observed, not proved)\n", days);
}

// Random arrivals with 3 chairs.
static void test_random_arrivals(void) {
    long days = harness_iters(10);
    for (long d = 0; d < days; d++)
        if (day(3, 40, 20000, 300, customer) < 0) return;
}

int main(void) {
    harness_watchdog(50);
    harness_hint("customer: down(mutex); if waiting == chairs -> balk; else waiting++, up(customers), "
                 "up(mutex), wait to be called, sit, wait for the cut to end. barber: down(customers); "
                 "down(mutex); waiting--; call a customer; up(mutex); learn its id; shop_haircut(id); "
                 "tell it it is done.");
    RUN_TEST(test_burst_with_one_chair);
    RUN_TEST(test_slow_trickle_nobody_turned_away);
    RUN_TEST(test_fast_arrivals_many_days);
    RUN_TEST(test_random_arrivals);
    return harness_report();
}
