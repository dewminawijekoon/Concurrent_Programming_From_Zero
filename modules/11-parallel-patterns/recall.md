# 11 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| When can a loop's iterations be split between threads? | When they give the same result in any order: no iteration writes something that a different iteration reads or writes. |
| What is a loop-carried dependence? Give an example. | A value flows from one iteration to a later one, e.g. `a[i] = a[i-1] + x[i]`. Such a loop cannot simply be split. |
| What is a reduction? | Combining all items into one value with an operation like + or max. Each thread keeps a private partial result, and the parts are combined at the end. |
| What is SPMD? | Single program, multiple data: every thread or process runs the same code and uses its rank to choose its part of the data (L10 slide 14). |
| Block, cyclic, block-cyclic: who owns item i? | Block: one run of consecutive items per thread (n/p each, the first n mod p get one extra). Cyclic: thread i mod p. Block-cyclic: thread (i / b) mod p. |
| Static vs dynamic load balancing? | Static: the split is fixed before the work starts; no run-time cost; fine when items cost the same. Dynamic: free threads take the next piece from a shared counter or queue; small cost per piece; needed when costs differ. |
| Why can cyclic partitioning still be unbalanced? | If the data has a pattern with the same period as the number of threads. Primes with 4 threads: two threads get only even numbers, which are trivial to test. |
| What does the chunk size trade off in dynamic scheduling? | Small chunks balance better but mean more trips to the shared counter. Big chunks are cheap to hand out but can leave threads idle at the end. |
| How does divide & conquer run on threads? | Fork a thread for one half, do the other half yourself, join, combine (L02's spawn and sync). |
| Why a threshold in divide & conquer? | Below it, the work is cheaper than starting a thread, so do it serially. Too low: too many threads, overhead wins. Too high: too few pieces, cores idle. |
| What is a pipeline? | Each item goes through stages in order; each stage is a thread; stages are connected by bounded queues. Several items are in progress at once. |
| Pipeline time for n items? | About (sum of stage times) + (n − 1) × (slowest stage time). With k equal stages of time t: (k + n − 1) × t. |
| What limits a pipeline's throughput? How do you fix it? | The slowest stage (the bottleneck). Make it faster or give it more threads (replicate it), until another stage becomes the slowest. |
| Two traps when a stage is replicated? | Items can come out of order (use sequence numbers and a reorder buffer); and you need one poison pill per copy of the stage. |
| Why bounded queues between stages? | So a fast stage waits instead of filling memory with items a slow stage cannot keep up with. |
| What is a thread pool and why use one? | A fixed set of workers taking tasks from one shared queue. No thread created per task, and fast workers automatically take more tasks (dynamic balancing). |
| Thread pool details that matter | Run the task with the lock released; count pending tasks from submit to finish; shut down by setting a flag and broadcasting. |
| How does work stealing work? | Each worker has its own deque. It pushes and pops at the back; when empty, it steals one task from the front of a random other worker's deque. |
| Why pop from the back but steal from the front? | Owner and thieves rarely want the same item; the newest tasks are still in the owner's cache; the oldest tasks are often the biggest, so steals are rare. |
| What is map-reduce? | Map: apply a function to every item independently. Reduce: combine the results (sum, max, …), via private partial results per thread. |
| Threads for a CPU-bound job on 4 cores? I/O-bound? | CPU-bound: about 4. I/O-bound: more, roughly cores × (1 + wait time / compute time). |
| Input bigger than memory? | Read and process it in chunks with a reusable buffer; carry the unfinished line at a chunk's end into the next chunk. |
| Many threads writing one shared output file? | They queue for the lock, so writing becomes serial. Use per-thread buffers and merge (or write in big blocks). |
| Read-only data shared by all threads? | Load it once before starting the threads; it needs no lock because nobody writes it. |

**From memory:** write the dynamic-scheduling worker loop (exercise 4).
Draw a 3-stage pipeline with times and work out the total time for 100
items, with and without replicating the slowest stage.
