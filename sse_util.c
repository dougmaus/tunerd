/* sse_util.c */

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
#include <string.h>

/* POSIX headers */

/* local headers */
#include "sse_util.h"
#include "sckt_util.h"

/*
This code module will handle sending a message "Data" at a set of open connections
These connections will be from browsers that are listening.
The code will keep track of the file descriptors for each listening connection
*/

/*
functions to:
- initialize
- create an eventsource descriptor
- add a socket to an eventsource
- send a data string to sockets of the eventsource
- remove a socket from eventsource
*/

/* preprocessor definitions */
#define MAX_SOCKETS 32 

/* structures */

/* maps listen sockets to eventsource descriptors */
struct socket_sse_map_struct {
 int count;
 int socket[MAX_SOCKETS];
 int sse[MAX_SOCKETS];
} socket_sse_map;


/****************/
/* sse_unique() */
/****************/
static int
sse_unique(void){
int i;
int cand = 0;
int present = 0;

  do {
    cand += 1;
    present = 0;
    for (i = 0; i < socket_sse_map.count; i++) {
      if (socket_sse_map.sse[i] == cand) {
        present = -1;
        break;
      }
    }
  } while (present == -1);

  return(cand);
}


/**************/
/* sse_init() */
/**************/
/* return: 0 on success */
int
sse_init(void)
{
  socket_sse_map.count = 0;

  return(0);
}


/*************/
/* sse_new() */
/*************/
/* takes a socket descriptor and starts a new sse */
/* return: descriptor of new eventsource, -1 on error */
int
sse_new(
 int in_socket)
{
int i = 0;

  /* sanity checks */
  i = socket_sse_map.count;
  if (i >= MAX_SOCKETS) {
    fprintf(stderr, "sse_new: exceeded maximum number of sockets\n");
    /* make no changes */
    return(-1);
  }

  socket_sse_map.socket[i] = in_socket;
  socket_sse_map.sse[i] = sse_unique();

  /* increment counters */
  socket_sse_map.count += 1;

  return(socket_sse_map.sse[i]);
}


/*************/
/* sse_add() */
/*************/
/* takes a socket descriptor, and a previously made sse (by sse_new) */
/* and associates socket to sse */
/* return: 0 on success, -1 on error (exceed max list size) */
int
sse_add(
 int in_sse_descriptor,
 int in_socket)
{
int i = 0; 

  i = socket_sse_map.count;
  if (i >= MAX_SOCKETS) {
    fprintf(stderr, "sse_add: exceeded maximum number of sockets\n");
    return(-1);
  }

  socket_sse_map.socket[i] = in_socket;
  socket_sse_map.sse[i] = in_sse_descriptor;

  /* increment counter */
  socket_sse_map.count += 1;

  return(0);
}


/*************/
/* sse_rem() */
/*************/
/* takes a socket descriptor and removes it from the map */
/* (searches the map for that socket) */
/* return: 0 on success, -1 on error (not found) */
int
sse_rem(
 int in_socket)
{
int oldcount = 0;
int i = 0;

  oldcount = socket_sse_map.count;

  for (i = 0; i < oldcount; i++) {
    if (socket_sse_map.socket[i] == in_socket) break;
  }
  if (i >= oldcount) {
    /* socket to remove not found, caller may have been fishing */
    return(-1);
  }
  while (i < (oldcount-1)) {
    socket_sse_map.socket[i] = socket_sse_map.socket[i+1];
    socket_sse_map.sse[i]    = socket_sse_map.sse[i+1];
    i++;
  }
  socket_sse_map.count -= 1;

  return(0);
}


/**************/
/* sse_send() */
/**************/
/* takes a sse_descriptor and a char array/string */
/* sends string to all sockets listening on that sse */
/* if disconnect is TRUE(nonzero), will also disconnect all sockets */
/* return: 0 on success */
int
sse_send(
 int in_sse_descriptor,
 const char *message,
 int disconnect)
{
int message_len = 0;
int fd = 0;
int send_status = 0;
int count = 0;
int i = 0;

  message_len = 0;
  if (message != NULL) {
    message_len = strlen(message);
  }

  count = socket_sse_map.count;

  if (message_len > 0) {
    for (i = 0; i < count; i++) {
      if (socket_sse_map.sse[i] == in_sse_descriptor) {
        fd = socket_sse_map.socket[i];
        send_status = sckt_write(fd, message, message_len);
      }
    }
  }

  if (disconnect) {
    /* in reverse order to minimize rearrangements */
    for (i = count; i > 0; i--) {
      if (socket_sse_map.sse[i-1] == in_sse_descriptor) {
        fd = socket_sse_map.socket[i-1];
        /* send disconnect to socket */
        sckt_close(fd);
        /* remove socket from map */
        sse_rem(fd);
      }
    }
  }

  return(send_status);
}
