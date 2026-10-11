# 14 · Quiz answers: distributed mutual exclusion [EXTRA]

These answers are about the length a marker expects for the marks shown.
"Why" is always part of the answer.

## A. True / false

1. **False.** Peterson's proof assumes a thread reads the *current* value of
   the other's flag. A message only tells you what the flag *was* when it
   was sent; by the time it arrives the other process may already have
   changed it and entered. Both processes can act on old information and
   both enter. You need an algorithm designed for messages.
2. **False.** Lamport clocks only promise the other direction. Two
   concurrent events can have any timestamps: in README section 3.1, g has
   2 and d has 3, but no chain of messages links them.
3. **True.** Each step of a happened-before chain increases the clock: a
   later event in the same process ticks it, and a receive sets it to
   more than the send's timestamp. So along any chain the timestamps
   strictly grow.
4. **True.** If it has just passed the token on when it starts wanting
   the critical section, the token must visit the other n − 1 processes
   first. (It never waits longer, so there is no starvation.)
5. **False.** A HELD process must **defer** every request and reply only
   when it leaves. If it replied at once, the requester could collect all
   replies and enter while it is still inside.
6. **True.** The coordinator needs 3 messages per entry (request, grant,
   release); Ricart–Agrawala needs 2(n − 1), which is more than 3 for every
   n > 2 (n = 3 gives 4).
7. **False.** Every request needs a reply from **every** other process. A
   crashed process never replies, so every later request waits forever. It
   has n points of failure instead of one.

## B. Calculations and traces

8. P0: local **1**, send m1 **2**. P1: send m2 **1**, local **2**. P2: local
   **1**; receive m2 → max(1, 1) + 1 = **2**; receive m1 → max(2, 2) + 1 =
   **3**; local **4**.
   Concurrent pairs, for example: P0's local event (1) and P1's send of m2
   (1); or P1's local event (2) and any event of P0 or P2 (no message
   leaves P1 after that local event, and none arrives at P1, so no chain
   links it to them).
9. (a) Coordinator: **3**. (b) Ricart–Agrawala: 2(n − 1) = **16**.
   (c) Grid: a quorum is the row plus the column, 3 + 3 − 1 = 5 processes
   including yourself, so 4 others; each gets a request, sends a grant and
   gets a release: 3 × 4 = **12**. (d) Token ring, everyone always wanting:
   every single pass brings the token to a process that uses it, so about
   **1 message per entry**. Only one process wanting: the token goes all the
   way round between its uses, **n = 9 messages per entry** (and the token
   keeps circulating even if nobody wants it at all).
10. 1. P1 (RELEASED) replies to both P0 and P2.
    2. P0 receives REQUEST(2, P2). P0 is WANTED with (4, P0); (2, P2) is
       earlier, so P0 **replies at once**.
    3. P2 receives REQUEST(4, P0). P2 is WANTED with (2, P2), which is
       earlier, so P2 **defers** P0.
    4. P2 has replies from P1 and P0: **P2 enters first**.
    5. P2 leaves and sends its deferred reply to P0.
    6. P0 now has replies from P1 and P2: **P0 enters second**.
    The earlier timestamp wins, whatever order the messages arrive in.
11. The timestamps are equal, so the ids decide: (5, P1) is earlier than
    (5, P3). P1 **defers** P3's request; P3, being WANTED with the later
    request (5, P3), **replies** to P1. So P1 enters, then replies to P3.
    Without the tie-break, the two requests are "equal": if "defer when
    mine is earlier or equal", both defer and wait for each other forever
    (**deadlock**); if "defer only when mine is strictly earlier", both reply
    and **both enter** (mutual exclusion broken).

## C. Proofs and design

12. **Mutual exclusion:** there is exactly one token, created once and
    never copied: a process that passes it on no longer has it. A process
    enters only while it holds the token, so at most one process is inside.
    **No starvation:** the token moves around the ring in a fixed order and
    every process passes it on after at most one critical section. So the
    token reaches any waiting process after at most n − 1 passes, each
    taking a finite time. Every waiting process therefore gets in.
13. Suppose, to get a contradiction, that Pi and Pj are inside at the same
    time. Both needed a reply from the other. Requests are totally ordered
    by (timestamp, id); name them so that (Ti, i) is earlier than (Tj, j).
    Consider when Pj's REQUEST(Tj, j) reached Pi.
    - *Pi had already requested* (WANTED or HELD with (Ti, i)). Its request
      is earlier, so Pi deferred Pj and replies only after leaving the
      critical section. So Pj cannot enter while Pi is inside.
    - *Pi had not yet requested.* Pi replied, but receiving the request set
      Pi's clock above Tj. Pi's later request then got Ti > Tj, so (Ti, i)
      is later than (Tj, j). That contradicts how we named them.
    Both cases are impossible, so Pi and Pj are never inside together.
14. The token disappears with the crashed process: no one can ever enter
    again, and the ring is broken at that process too. Recovery: the others
    must detect the loss (for example, "no token seen for a long time"),
    rebuild the ring around the dead process, and create a **new** token.
    Danger: the token may only be slow, not lost; if a new one is created,
    there are **two tokens** and two processes can be inside at once. So
    the processes must agree that exactly one process creates the new token
    (an election), and old tokens must be recognisable (for example, a
    generation number).
15. Other processes may still send it REQUESTs, and they cannot enter
    without its REPLY. If it stopped, they would wait forever. In exercise
    6 each process sends DONE to all others after its last entry, and keeps
    serving messages until it has finished its own entries **and** received
    DONE from every other process. Because MPI does not let a later message
    from one sender overtake an earlier one, a DONE arrives after all of
    that sender's requests, so nobody is left waiting.
16. **Coordinator** (for example on the print server itself). Message cost:
    3 per print job, against 2 × 49 = 98 for Ricart–Agrawala; the token ring
    would send the token round 50 machines all day long even though
    printing is rare. Failures: if the coordinator runs on the print
    server, its crash also means the printer is gone, so the single point
    of failure costs nothing extra. Ricart–Agrawala would stop whenever any
    one of 50 office computers is switched off, and the token ring breaks
    whenever a machine in the ring is switched off.
17. Every two quorums share at least one process, and a process says yes
    (grants) to only one requester at a time, until that requester
    releases. Two processes inside together would both hold a yes from the
    shared member, which is impossible. **Deadlock:** P1's and P2's quorums
    share members A and B. A says yes to P1 and B says yes to P2 at the same
    time. P1 waits for B, P2 waits for A, and each member is waiting for
    its requester's release: a cycle of waiting, as in module 10.
