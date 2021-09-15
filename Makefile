.PHONY: all clean test

PROG=   tcpexec

CFLAGS ?= -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-strong \
          -Wformat -Werror=format-security \
          -fno-strict-aliasing
LDFLAGS += -Wl,-z,relro,-z,now

RM ?= rm

TCPEXEC_CFLAGS ?= -g -Wall -fwrapv

CFLAGS += $(TCPEXEC_CFLAGS)

LDFLAGS += $(TCPEXEC_LDFLAGS)

all: $(PROG)

$(PROG):
	$(CC) $(CFLAGS) -o $@ $@.c $(LDFLAGS)

clean:
	-@$(RM) $(PROG)

test: $(PROG)
	@PATH=.:$(PATH) bats test
