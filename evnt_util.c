/* evnt_util.c */

/*
 * Copyright (c) 2017 Douglas Maus
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/* Feature test switches */
#define _POSIX_C_SOURCE 200112L

/* System headers */
/* C language headers */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>

/* POSIX headers */
/*  issue 1 */
#include <unistd.h>
/*  issue 4 */
#include <poll.h>

/* Local headers */
#include "evnt_util.h"
#include "sckt_util.h"
#include "http_util.h"
#include "sse_util.h"

/* Macros */
#ifndef RBUFSIZE
#define RBUFSIZE 16384 
#endif

/* File scope variables */
static int stop_server = 0; /* 0 false, continue, -1 true, stop */
static int listen_count = 0;
 /* number of sockets on the primary listen port 80, or 8080 */
 /*  for possibility of IPv4 and/or IPv6 */
 /*  other sockets after accept */
 /*  are connected to another (ephemeral) port */
static int max_connections = 0;

static unsigned int    polld_size  = 0;
static unsigned int    polld_count = 0;
static struct pollfd  *polld_array = NULL;

/* External variables */
/* External functions */

/* Structures and unions */
/* polld array has to be constantly compacted */

/*  map_poll_buf maps the index of poll array (file descriptor) */
/*  to the index of buffer fd_buf for that file descriptor */
static signed int *map_poll_buf = NULL;

/* fd_buf is an array of buffers for holding HTTP requests */
struct fd_buf_struct {
 int fd;
 unsigned int pos;
 char buf[RBUFSIZE];
};
static struct fd_buf_struct *fd_buf = NULL;


/* Signal catching functions */
/* note: signal() is deprecated, sigaction() is preferred */

/**********************/
/* interruptHandler() */
/**********************/
static void
interruptHandler(
 int signum)
{
  stop_server = 1;
}


/* Functions */


/***************/
/* polld_add() */
/***************/
/* return: 0 on success, -1 error */
static int
polld_add(
 int in_fd)
{
int i = 0;

  if (polld_count >= polld_size) {
    fprintf(stderr, "polld_add: exceeds max poll array size\n");
    return(-1);
  }

  /* find an unused buffer */
  for (i = 0; i < max_connections; i++) {
    if (fd_buf[i].fd == (-1) ) break;
  }
  if (i == max_connections) {
    fprintf(stderr, "polld_add: error, no buffers available?\n");
    return(-1);
  }

  /* activate and reset buffer */
  fd_buf[i].fd = in_fd;
  fd_buf[i].pos = 0;
  fd_buf[i].buf[0] = '\0';

  /* map poll array index to buffer index */
  map_poll_buf[polld_count] = i;

  /* append fd onto polld array */
  polld_array[polld_count].fd = in_fd;
  polld_array[polld_count].events = POLLIN;
  polld_array[polld_count].revents = 0;

  polld_count += 1;

  return(0);
}


/***************/
/* polld_rem() */
/***************/
/* return: 0 on success, -1 error */
static int
polld_rem(
 int in_fd)
{
int i = 0;
int m = 0;

  /* find file_descriptor */
  for (i = listen_count; i < polld_count; i++) {
    if (polld_array[i].fd == in_fd) break;
  }
  if (i == polld_count) {
    fprintf(stderr, "polld_rem: descriptor not found\n");
    return(-1);
  }

  m = map_poll_buf[i];

  /* de-activate buffer associated with fd */
  fd_buf[m].fd = (-1);
  fd_buf[m].pos = 0;
  fd_buf[m].buf[0] = '\0';

  /* remove, compact polld array by overwriting and shifting down */
  polld_count -= 1;
  while (i < polld_count) {

    polld_array[i].fd = polld_array[i+1].fd;
    polld_array[i].events = polld_array[i+1].events;
    polld_array[i].revents = polld_array[i+1].revents;

    map_poll_buf[i] = map_poll_buf[i+1];

    i += 1;
  }

  return(0);
}


/***************/
/* evnt_init() */
/***************/
/* return: 0 success, -1 error */
int
evnt_init(
 int in_fd4,
 int in_fd6,
 unsigned int in_max_connections)
{
struct sigaction sa;
int i = 0;

  /* register signal action handler for SIGINT */
  sa.sa_handler = interruptHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGINT, &sa, NULL) == (-1)) {
    fprintf(stderr, "evnt_init: sigaction() error for SIGINT\n");
  }
  if (sigaction(SIGTERM, &sa, NULL) == (-1)) {
    fprintf(stderr, "evnt_init: sigaction() error for SIGTERM\n");
  }

  /* input checking */
  max_connections = in_max_connections;
  if (in_fd4 < 0 && in_fd6 < 0) {
    fprintf(stderr, "evnt_init: no listen sockets\n");
    return(-1);
  } else if (in_fd4 >= 0 && in_fd6 >= 0) {
    listen_count = 2;
    polld_size = max_connections + 2;
  } else {
    listen_count = 1;
    polld_size = max_connections + 1;
  }

  /* allocate polld_array */
  polld_array = malloc(sizeof(struct pollfd) * polld_size);
  if (polld_array == NULL) {
    fprintf(stderr, "evnt_init: malloc() for poll error\n");
    return(-1);
  }
  polld_count = 0;
  /* and put in listen file descriptors */
  if (in_fd4 >= 0) {
    polld_array[polld_count].fd = in_fd4;
    polld_array[polld_count].events = POLLIN;
    polld_count += 1;
  }
  if (in_fd6 >= 0) {
    polld_array[polld_count].fd = in_fd6;
    polld_array[polld_count].events = POLLIN;
    polld_count += 1;
  }

  /* allocate for map from polld array index to buffer index */
  /*  and read buffers */
  map_poll_buf = malloc(sizeof(signed int) * polld_size);
  if (map_poll_buf == NULL) {
    fprintf(stderr, "evnt_init: malloc() for map error\n");
    return(-1);
  }
  for (i = 0; i < polld_size; i++) {
    map_poll_buf[i] = (-1);
  }
  fd_buf = malloc(sizeof(struct fd_buf_struct) * max_connections);
  if (fd_buf == NULL) {
    fprintf(stderr, "evnt_init: malloc() for fd_buf error\n");
    return(-1);
  }
  for (i = 0; i < max_connections; i++) {
    fd_buf[i].fd = (-1);
    fd_buf[i].buf[0] = '\0';
    fd_buf[i].pos = 0;
  }

  return(0);
}


/***************/
/* evnt_loop() */
/***************/
int
evnt_loop(void)
{
char *buf = NULL;
ssize_t nr = 0;
int i = 0;
int poll_status = 0;
int acpt_fd = 0;
int fd = 0;
int close_code = 0;
int rem = 0;
unsigned int m = 0;
unsigned int p = 0;

  while(stop_server == 0) {

    poll_status = poll(polld_array, polld_count, 100);

    if (poll_status == (-1)  ) {
      /* either poll() error */
      /*  or poll() interrupted by SIGINT, interruptHandler called */
      /*  and control returned to poll(), with errno set to EINTR */
      if (errno != EINTR) {
        fprintf(stderr, "evnt_loop: poll() error\n");
        stop_server = 1;
      }
    } else if (poll_status == 0) {
      /* timed out, idle */
    } else {
      /* event to handle */

      /* loop through all the poll descriptors */
      for (i = 0; i < polld_count; i++) {

        /* skip descriptors without events */
        if (polld_array[i].revents == 0) {
          continue;
        }

        if (i < listen_count) {
          /* handle connection on listen socket */

          acpt_fd = sckt_accept(polld_array[i].fd);
          if (acpt_fd == (-1)) {
            fprintf(stderr, "evnt_loop: accept() error\n");
            stop_server = 1;
          } else {
            if (polld_add(acpt_fd) == (-1)) {
              sckt_close(acpt_fd);
            }
          }

        } else {
          /* handle non-listen socket event */

          /* read into buffer */
          fd = polld_array[i].fd;
          m = map_poll_buf[i];
          p = fd_buf[m].pos;
          buf = &(fd_buf[m].buf[p]);
          rem = (RBUFSIZE -1) - p;
          while ((nr = sckt_read(fd, buf, rem)) > 0) {
            rem -= nr;
            buf += nr;
            fd_buf[m].pos += nr;
          }
          *buf = '\0'; /* zero terminate string */

          /* has read until nr is -1 (EAGAIN or error) */
          /*  or 0 (client closed connection or buf full) */

          close_code = 0; /* default is 0 close, -1 for keep-alive */
                          /* +1 for client already closed socket */

          if (nr == (-1)) {
            if (errno == EAGAIN) {
              close_code = http_handle(fd_buf[m].buf, fd_buf[m].fd);
            } else {
              fprintf(stderr, "evnt_loop: read() error\n");
            }
          } else {
            if (rem > 0) {
              /* client closed connection */
              close_code = 1;
            } else {
              fprintf(stderr, "evnt_loop: read buffer exceeded\n");
            }
          }

          if (close_code >= 0) {
            if (close_code == 0) {
              sckt_close(polld_array[i].fd);
            }
            /* definitely remove from poll array */
            polld_rem(polld_array[i].fd);
            /* no way to know if SSE or not, try to remove */
            sse_rem(polld_array[i].fd);
          }

        }

      }

    }
    
  }

  return(0);
}


/**************/
/* evnt_end() */
/**************/
void
evnt_end(void)
{
int i = 0;

  i = listen_count;
  while (i < polld_count) {
    sckt_close(polld_array[i].fd);
    polld_rem(polld_array[i].fd);
    i += 1;
  }

  free(polld_array);
  free(map_poll_buf);
  free(fd_buf);
}
