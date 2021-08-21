# SYNOPSIS

tcpexec [*OPTION*] [*IPADDR:*]*PORT* *COMMAND* *...*

# DESCRIPTION

tcpexec: a minimal, [UCSPI](https://jdebp.uk/FGA/UCSPI.html) inetd

`tcpexec` attaches the standard input and output of a command to a
TCP socket:

* exec(3): data is not proxied via an intermediary process

* `SO_REUSEPORT`: multiple processes concurrently listen and accept data
  on the same port

# EXAMPLES

## echo server

```
$ tcpexec 9090 cat

$ tcpexec 127.0.0.1:9090 cat

$ tcpexec ::1:9090 env
```

## Supervised using daemontools

An echo server allowing 3 concurrent connections:

    service/
    ├── echo1
    │   └── run
    ├── echo2
    │   └── run
    └── echo3
        └── run

*  service/echo1/run

```
#!/bin/sh

exec tcpexec 127.0.0.1:9090 cat
```

* service/echo2/run

```
#!/bin/sh

exec tcpexec 127.0.0.1:9090 cat
```

* service/echo3/run

```
#!/bin/sh

exec tcpexec 127.0.0.1:9090 cat
```

Then run:

    svscan service

# Build

    make

    #### static executable using musl
    ./musl-make

# OPTIONS

-v, --verbose
: write additional messages to stderr

-h, --help
: usage summary

# ENVIRONMENT VARIABLES

PROTO
: protocol, always set to TCP

TCPREMOTEIP
: source IPv4 or IPv6 address

TCPREMOTEPORT
: source port

TCPLOCALIP
: destination IPv4 or IPv6 address

TCPLOCALPORT
: destination port
