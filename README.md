# Atomic SWAR Token Buckets

[tokenbucket.c](src/tokenbucket.c) provides multithreaded lockless SWAR
(SIMD within a register) token bucket implementation using C11 atomics.
It's used by the [redbean web server](https://redbean.dev/) to protect
our self-hosted production services from DDOS attacks. Based on our
experience, it foils attackers much more effectively than Cloudflare.

The basic idea here is you can allocate an `int8_t` array with 2**24
items. Those are your buckets, where each bucket represents a block of
256 IP addresses, all of whom are assigned 127 tokens. Each time an IP
sends us a request, it needs to take a token from its bucket. If the
bucket is empty, the client gets added to the raw prerouting firewall
table. Each second, the single backround replenish worker adds one new
token to each bucket. Many threads consume them.

The tradeoff with token bucket has always been the cost of running a
persistent background process that's always active. By using SWAR, this
library is able to address that issue by offering optimal performance.
For example, it should take about 212 microseconds to replenish 2**22
buckets if all of them are full. If none of them are full, then it'll
take about 2,000 microseconds, due to atomic operations kicking in. On
the other hand, a naive implementation of this algorithm would take
15,805 microseconds to replenish the same number of buckets.

The beautiful thing about the token bucket algortihm is that the default
state is a safe state. If there's a partial failure in your app and the
background replenisher stops running, then rather than losing protection
the token bucket algorithm will maximize protection by halting traffic,
similar in spirit to some of the most marvelous examples of engineering
in the past, such as George Westinghouse's air brake design, or Elisha
Otis' solution to elevator safety under cable failure.
