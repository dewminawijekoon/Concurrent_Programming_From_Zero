# 16 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Why is the work per item uneven in a prime-based problem? | Trial division rejects even numbers and numbers with small factors almost at once, but a prime p costs about √p / 2 divisions, which grows with p. |
| What does "imbalance" mean, and why does it matter? | Largest work of any worker divided by the average. The job ends when the slowest worker ends, so the speedup is at most T / imbalance. |
| What goes wrong with a block split here? | The last block holds the largest, most expensive numbers (L01 slides 17–19). |
| What goes wrong with a cyclic split here? | With an even number of workers some workers get only even numbers; other worker counts line up with other patterns (remainders mod 3). |
| What fixes it? | Dynamic scheduling (take the next chunk when free) or block-cyclic with medium blocks. |
| How does the chunk size matter? | Too small: overhead on the shared counter (or one MPI message round trip per chunk). Too big: back to static imbalance. Medium chunks or `guided` get both. |
| Pthreads dynamic version? | Shared atomic counter; each thread loops `start = atomic_fetch_add(&next, CHUNK)` until start ≥ M; private counts added after `pthread_join`. |
| OpenMP version? | `#pragma omp parallel for reduction(+:count) schedule(dynamic, 64)`. |
| OpenMP schedules ↔ strategies? | `static` = block; `static,1` = cyclic; `static,b` = block-cyclic; `dynamic,c` = shared counter; `guided` = shrinking chunks. |
| MPI dynamic version? | Manager–worker: workers send READY, the manager (`MPI_ANY_SOURCE`) replies with a chunk start or −1; at the end `MPI_Reduce` to rank 0. |
| CUDA version? | Grid-stride loop over items, private counter, one `atomicAdd` per thread; host: malloc, memset, launch, check, copy back, free. |
| GPU-specific balance problem? | Warp divergence: a warp waits for its slowest thread; skipping even numbers helps. |
| How are the partial counts combined in each paradigm? | Pthreads: after join. OpenMP: reduction. MPI: `MPI_Reduce`. CUDA: private count + `atomicAdd` (or block reduction). |

**From memory:** write the Pthreads dynamic version (exercise 6) and the
5-mark load-balancing justification (README section 5.1).
