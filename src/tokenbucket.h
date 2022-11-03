#ifndef TOKENBUCKET_H_
#define TOKENBUCKET_H_
#include <stdatomic.h>
#include <stdint.h>
#include <stddef.h>

void ReplenishTokens(atomic_uint_fast64_t *, size_t);
int AcquireToken(atomic_schar *, uint32_t, int);
int CountTokens(atomic_schar *, uint32_t, int);

#endif /* TOKENBUCKET_H_ */
