# 06 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Write the Filter lock | `lock(i): for L = 1..n-1 { level[i] = L; victim[L] = i; while (some k != i has level[k] >= L && victim[L] == i) {} }` and `unlock(i): level[i] = 0` |
| How many levels does Filter have, and what is the key claim? | n − 1 levels. At most n − L threads get past level L, so at most 1 gets past level n − 1. |
| Why can't n − L + 1 threads pass level L? | The one that wrote `victim[L]` last sees all the others at level L or higher, and sees itself as the victim, so it must wait. |
| Is Filter starvation-free? First-come-first-served? | Starvation-free: yes, each level works like Peterson. First-come-first-served: no, a thread can be overtaken by one that arrived later. |
| What is the doorway? | The first part of `lock()`, which a thread always finishes in a bounded number of its own steps, without waiting. |
| Bounded waiting vs first-come-first-served? | Bounded waiting: after A's doorway, others can go ahead of A at most r times. First-come-first-served: if A finishes its doorway before B starts its doorway, A goes first (r = 0). |
| Write the Bakery lock | `lock(i): flag[i] = true; label[i] = max(label) + 1; while (some k != i has flag[k] && (label[k], k) < (label[i], i)) {}` and `unlock(i): flag[i] = false` |
| What does (a, i) < (b, j) mean? | a < b, or a == b and i < j. |
| Why break ties by id? | `max()` is not atomic, so two threads can get the same label. The ids differ, so (label, id) pairs never tie. |
| What if Bakery's `max()` is not atomic? | Still correct: equal labels are possible, and the id breaks the tie. The flag, raised before reading, is what makes it safe. |
| Bakery's weak point? | Labels grow forever, so a fixed-size integer would eventually overflow. |
| Name the hardware read-modify-write instructions | test-and-set, atomic exchange, fetch-and-increment, compare-and-swap, load-linked/store-conditional |
| What does test-and-set do? | Sets the variable to true and returns the old value, in one step. Old value false means "it was free and now it is mine". |
| What does CAS do? | If x equals `expected`, set x to `new` and return true; otherwise change nothing and return false. All in one step. |
| Write a TAS lock and a TTAS lock | TAS: `while (TAS(held)) {}`. TTAS: `loop { while (held) {}; if (!TAS(held)) return; }`. Unlock: `held = false`. |
| Why is TTAS faster than TAS? | Every TAS is a write, which invalidates all other cached copies of the lock. TTAS waits with reads, which hit the core's own shared copy and make no traffic until the lock is released. |
| What does cache coherence promise? | A read returns the latest written value, and all cores see the writes to one location in the same order. |
| Write-invalidate vs write-update? | Invalidate: other copies are thrown away on a write (common, less traffic). Update: the new value is sent to every copy (more bandwidth). |
| Snooping vs directory? | Snooping: all caches listen on a shared bus (simple, few cores). Directory: one place records who has each line and messages only them (scales). |
| What is false sharing? | Different variables used by different cores sit in the same cache line, so each write invalidates the other cores' copies. Fix: padding. |
| TAS lock on one core with priorities: what goes wrong? | The low-priority holder is preempted; the high-priority thread spins, is always ready, so the holder never runs again and never unlocks: livelock. Fix: block instead of spinning. |
| What if test-and-set is not atomic? | Two threads can both read 0 and both enter; or a waiter's write of 1 lands after the holder's release, so the lock stays "taken" with nobody inside and everyone spins forever. |
| Spin or block? | Spin when waits are very short and there are spare cores. Block when waits are long, threads outnumber cores, or there are priorities on one core. |

**From memory:** write Filter and Bakery in pseudocode (exercise 3 for
Bakery in C). Write a TTAS lock. Explain the one-core priority problem in
four sentences.
