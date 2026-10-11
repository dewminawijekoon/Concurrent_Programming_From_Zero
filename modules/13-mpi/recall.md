# 13 · Recall sheet

Cover the right-hand column. Say each answer out loud, or write it on a
blank page. If you hesitate, re-read that part of the [README](README.md).

| Question | What you should be able to say or write |
|---|---|
| Why do we need MPI? | On a cluster each computer has its own memory, so processes cannot share variables. They must send messages; MPI is the standard library for that. |
| What is SPMD? | Single Program, Multiple Data: one program, started as many processes; each uses its rank in `if`s to do its own part. |
| Compile and run? | `mpicc -g -Wall -o prog prog.c`, then `mpiexec -n 4 ./prog`. |
| The four calls every MPI program has? | `MPI_Init(&argc, &argv)`, `MPI_Comm_size(MPI_COMM_WORLD, &p)`, `MPI_Comm_rank(MPI_COMM_WORLD, &my_rank)`, `MPI_Finalize()`. |
| What is a communicator? | A group of processes that can talk to each other; `MPI_COMM_WORLD` holds all of them; ranks go 0 .. size−1. |
| Arguments of `MPI_Send`? | buffer, count (items), datatype, destination rank, tag, communicator. |
| Arguments of `MPI_Recv`? | buffer, max count, datatype, source rank (or `MPI_ANY_SOURCE`), tag (or `MPI_ANY_TAG`), communicator, status (or `MPI_STATUS_IGNORE`). |
| When does a receive match a send? | Same communicator, the right source (or any), the right tag (or any). |
| How do you find the sender and size after `MPI_ANY_SOURCE`? | `status.MPI_SOURCE`, `status.MPI_TAG`, and `MPI_Get_count(&status, type, &count)`. |
| Does `MPI_Send` wait? Does `MPI_Recv`? | Send may buffer and return, or wait for the receiver (big messages); never rely on either. Recv always waits for a matching message. |
| Message order? | From one sender to one receiver on one communicator: no overtaking. Between different senders: any order. |
| Trapezoid rule plan? | Each rank takes a block of strips, computes its area alone, then the pieces are added on rank 0 (Send/Recv, or `MPI_Reduce`). |
| Tree global sum? | In round s (d = 2ˢ), ranks that are multiples of 2d receive from rank + d; ranks at d past a multiple of 2d send to rank − d. ⌈log₂ p⌉ rounds, p − 1 messages. |
| Butterfly? | In round s every rank swaps with rank XOR 2ˢ and adds; after log₂ p rounds everyone has the total (an Allreduce). |
| Arguments of `MPI_Reduce`? | sendbuf, recvbuf, count, datatype, op (e.g. `MPI_SUM`), root, communicator. Only the root gets the result. |
| Rules for collectives? | All processes call the same one, in the same order, with compatible arguments (same root); no tags. |
| Allreduce, Bcast, Scatter, Gather, Allgather? | Everyone gets the combined result; root's data copied to all; root's array cut into pieces, piece r to rank r; pieces collected on root in rank order; pieces collected on everyone. |
| Block, cyclic, block-cyclic? | Neighbours together; deal one by one (i % p); deal blocks of b ((i / b) % p). |
| Block rule with leftovers? | q = n / p, extra = n % p; count = q + (r < extra); first = r·q + min(r, extra). |
| Send/send deadlock and fixes? [EXTRA] | Both send a big message first, both wait forever. Fix: order by even/odd rank, `MPI_Sendrecv`, or `MPI_Isend`/`MPI_Irecv` + `MPI_Waitall`. |
| How to remove `a[i] = a[i-1] + i`? [EXTRA] | Closed form a[i] = i(i+1)/2, or a prefix sum: local pass, `MPI_Scan` of block totals, add the lower ranks' total. |

**From memory:** write exercise 11 (sum over 1..N, result on rank 0) and the
trapezoid program with `MPI_Reduce`, with every argument correct.
