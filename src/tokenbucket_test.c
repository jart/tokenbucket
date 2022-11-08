/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "tokenbucket.h"
#include "vigna.h"

#define TB_CIDR   10  // 22 or 24 are better values for production
#define TB_BYTES  (1u << TB_CIDR)
#define TB_WORDS  (TB_BYTES / 8)
#define TB_TOKENS (TB_BYTES * 127)

atomic_int done;
atomic_int killed;

union TokenBucket {
  atomic_schar *b;
  atomic_uint_fast64_t *w;
} tok;

TEST(tokenbucket, test) {
  ASSERT_NE(NULL, (tok.b = calloc(TB_BYTES, 1)));
  ASSERT_EQ(0, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ASSERT_EQ(0, AcquireToken(tok.b, 0x7f000002, TB_CIDR));
  ReplenishTokens(tok.w, TB_WORDS);
  ReplenishTokens(tok.w, TB_WORDS);
  ASSERT_EQ(2, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ASSERT_EQ(1, AcquireToken(tok.b, 0x7f000002, TB_CIDR));
  ASSERT_EQ(0, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ASSERT_EQ(0, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ASSERT_EQ(2, AcquireToken(tok.b, 0x08080808, TB_CIDR));
  ReplenishTokens(tok.w, TB_WORDS);
  ASSERT_EQ(1, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ReplenishTokens(tok.w, TB_WORDS);
  ASSERT_EQ(1, AcquireToken(tok.b, 0x7f000001, TB_CIDR));
  ASSERT_EQ(0, AcquireToken(tok.b, 0x7f000002, TB_CIDR));
  ASSERT_EQ(3, AcquireToken(tok.b, 0x08080808, TB_CIDR));
  for (int i = 0; i < 130; ++i) ReplenishTokens(tok.w, TB_WORDS);
  ASSERT_EQ(127, AcquireToken(tok.b, 0x08080808, TB_CIDR));
  free(tok.b);
}

void OnTimeout(int sig) {
  killed = 1;
}

// single thread that adds a token to each bucket periodically
void *Replenisher(void *arg) {
  while (!done) {
    ReplenishTokens(tok.w, TB_WORDS);
    usleep(1);
  }
  return 0;
}

// many threads take a token whenever they to do something
void *Worker(void *arg) {
  int me, had;
  uint32_t ip;
  me = (intptr_t)arg;
  do {
    ip = Vigna();
    had = AcquireToken(tok.b, ip, TB_CIDR);
    ASSERT_EQ(0, had < -64);
  } while (had > 1 && !killed);
  return 0;
}

// make MODE=tsan
// hammer the buckets to see if tsan complains
TEST(tokenbucket, hammer) {
  long i, n = 8;
  pthread_t rp, id[n];
  alarm(10);
  signal(SIGALRM, OnTimeout);
  tok.b = malloc(TB_BYTES);      // allocate buckets
  memset(tok.b, 127, TB_BYTES);  // fill all buckets
  pthread_create(&rp, 0, Replenisher, 0);
  for (i = 0; i < n; ++i) pthread_create(id + i, 0, Worker, (void *)i);
  for (i = 0; i < n; ++i) pthread_join(id[i], 0);
  done = 1;
  pthread_join(rp, 0);
  free(tok.b);
  ASSERT_EQ(0, killed);
}
