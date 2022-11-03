#-*-mode:makefile-gmake;indent-tabs-mode:t;tab-width:8;coding:utf-8-*-┐
#───vi: set et ft=make ts=8 tw=8 fenc=utf-8 :vi───────────────────────┘

ifeq ($(MODE), tsan)
CC = c -O -pthread -fsanitize=thread
endif

all: o/$(MODE)/src/tokenbucket_test.com.runs
clean:; rm -rf o

o/$(MODE)/src/tokenbucket.o: src/tokenbucket.c src/tokenbucket.h Makefile
o/$(MODE)/src/tokenbucket_test.o: src/tokenbucket_test.c src/tokenbucket.h src/test.h src/vigna.h Makefile
o/$(MODE)/src/vigna.o: src/vigna.c src/vigna.h Makefile

o/$(MODE)/src/tokenbucket_test.com:				\
		o/$(MODE)/src/tokenbucket_test.o		\
		o/$(MODE)/src/tokenbucket.o			\
		o/$(MODE)/src/vigna.o
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

o/$(MODE)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

o/$(MODE)/%.com.runs: o/$(MODE)/%.com
	$<
	@touch $@
