/* Copyright (c) 2021, Michael Santos <michael.santos@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TCPEXEC_VERSION "0.1.1"

typedef struct {
  int verbose;
} tcpexec_state_t;

static const struct option long_options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static int tcpexec_listen(const char *addr, const char *port);
static int conninfo(const tcpexec_state_t *tp, const struct sockaddr *sa);
static void usage(void);

int main(int argc, char *argv[]) {
  tcpexec_state_t tp = {0};
  int lfd;
  int fd;
  struct sockaddr_storage sa;
  socklen_t salen = sizeof(sa);
  char *addr;
  char *port;
  char *p;
  int ch;

  while ((ch = getopt_long(argc, argv, "+hv", long_options, NULL)) != -1) {
    switch (ch) {
    case 'v':
      tp.verbose++;
      break;
    case 'h':
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 2) {
    usage();
  }

  /* 8888
   * :8888
   * 127.0.0.1:8888
   * ::1:8888
   * [::1]:8888
   */
  p = argv[0];
  addr = argv[0];

  if (*addr == '[') {
    addr++;
    p = strchr(addr, ']');
    if (p == NULL) {
      usage();
    }
    *p++ = '\0';
  }

  port = strrchr(p, ':');
  if (port == NULL) {
    port = addr;
    addr = "::";
  } else {
    *port++ = '\0';
    if (*addr == '\0') {
      addr = "::";
    }
  }

  lfd = tcpexec_listen(addr, port);
  if (lfd < 0)
    err(111, "listen: %s:%s", addr, port);

  fd = accept(lfd, (struct sockaddr *)&sa, &salen);
  if (fd < 0)
    err(111, "accept");

  if (conninfo((const tcpexec_state_t *)&tp, (const struct sockaddr *)&sa) < 0)
    err(111, "conninfo");

  if ((dup2(fd, STDOUT_FILENO) < 0) || (dup2(fd, STDIN_FILENO) < 0))
    err(111, "dup2");

  (void)execvp(argv[1], argv + 1);

  exit(127);
}

static int tcpexec_listen(const char *addr, const char *port) {
  struct addrinfo hints = {0};
  struct addrinfo *res;
  struct addrinfo *rp;
  int s;
  int fd = -1;

  int enable = 1;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  s = getaddrinfo(addr, port, &hints, &res);
  if (s != 0) {
    (void)fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    errno = EINVAL;
    return -1;
  }

  for (rp = res; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);

    if (fd < 0)
      continue;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
      return -1;

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) < 0)
      return -1;

    if (bind(fd, rp->ai_addr, rp->ai_addrlen) < 0)
      return -1;

    if (listen(fd, SOMAXCONN) >= 0)
      break;

    if (close(fd) < 0)
      return -1;
  }

  freeaddrinfo(res);

  if (rp == NULL)
    return -1;

  return fd;
}

static int conninfo(const tcpexec_state_t *tp, const struct sockaddr *sa) {
  char addrstr[INET6_ADDRSTRLEN] = {0};
  char portstr[6];
  int rv;

  if (setenv("PROTO", "TCP", 1) == -1) {
    return -1;
  }
  if ((unsetenv("TCPLOCALHOST") == -1) || (unsetenv("TCPREMOTEHOST") == -1) ||
      (unsetenv("TCPREMOTEINFO") == -1)) {
    return -1;
  }
  switch (sa->sa_family) {
  case AF_INET:
    rv = snprintf(portstr, sizeof(portstr), "%u",
                  ntohs(((const struct sockaddr_in *)sa)->sin_port));
    if (rv < 0 || rv > sizeof(portstr)) {
      return -1;
    }
    if (tp->verbose > 0)
      (void)fprintf(stderr, "%s:%s\n",
                    inet_ntoa(((const struct sockaddr_in *)sa)->sin_addr),
                    portstr);
    if (setenv("TCPREMOTEIP",
               inet_ntoa(((const struct sockaddr_in *)sa)->sin_addr),
               1) == -1) {
      return -1;
    }
    if (setenv("TCPREMOTEPORT", portstr, 1) == -1) {
      return -1;
    }
    return 0;
  case AF_INET6:
    rv = snprintf(portstr, sizeof(portstr), "%u",
                  ntohs(((const struct sockaddr_in6 *)sa)->sin6_port));
    if (rv < 0 || rv > sizeof(portstr)) {
      return -1;
    }
    (void)inet_ntop(AF_INET6, &(((const struct sockaddr_in6 *)sa)->sin6_addr),
                    addrstr, sizeof(addrstr));
    if (tp->verbose > 0)
      (void)fprintf(stderr, "[%s]:%s\n", addrstr, portstr);
    if (setenv("TCPREMOTEIP", addrstr, 1) == -1) {
      return -1;
    }
    if (setenv("TCPREMOTEPORT", portstr, 1) == -1) {
      return -1;
    }
    return 0;
  default:
    break;
  }

  errno = EAFNOSUPPORT;
  return -1;
}

static void usage() {
  errx(EXIT_FAILURE,
       "[OPTION] [<IPADDR>:]<PORT> <COMMAND> <...>\n"
       "version: %s\n"
       "-v, --verbose             write additional messages to stderr\n"
       "-h, --help                usage summary",
       TCPEXEC_VERSION);
}
