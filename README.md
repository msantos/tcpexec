# SYNOPSIS

tcpexec *[OPTION]* *[<IPADDR>:]<PORT>* *<COMMAND>* *<...>*

# DESCRIPTION

tcpexec: a minimal, [UCSPI](https://jdebp.uk/FGA/UCSPI.html) inetd

`tcpexec` attaches the stdin/stdout of a command to a TCP socket:

* immediately exec(3)'s the command: data is not proxied between file
  descriptors

* uses `SO_REUSEPORT` to allow processes to listen concurrently on the
  same port

# EXAMPLES

## echo server

```
$ tcpexec 9090 cat
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

``` service/echo1/run
#!/bin/sh

exec tcpexec 127.0.0.1:9090 cat
```

``` service/echo2/run
#!/bin/sh

exec tcpexec 127.0.0.1:9090 cat
```

``` service/echo3/run
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
