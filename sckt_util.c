/* sckt_util.c */

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
 /* for POSIX 1003.1-2004, issue 6, sockets */

/* System headers */
/* C language headers */
#include <stdlib.h>
#include <stdio.h>

/* POSIX headers */
/*  issue 1 */
#include <fcntl.h>      /* fcntl */
#include <sys/types.h>  /* read, ssize_t */
#include <unistd.h>     /* close */
/*  issue 6 */
#include <arpa/inet.h>  /* htons, inet_pton */
#include <netinet/in.h>
#include <sys/socket.h> /* socket, bind, listen, setsockopt */

/* Local headers */
#include "sckt_util.h"

/* Macros */
#ifndef BACKLOG
#define BACKLOG 2
#endif

/* File scope variables */
/* External variables */
/* External functions */
/* Structures and unions */
/* Signal catching functions */

/* Functions */

/******************/
/* sckt4_listen() */
/******************/
int
sckt4_listen(
 const char in_ipv4_addr[],
 unsigned short in_port)
{
int filedesc4 = 0;
int fcntl_flags = 0;
struct sockaddr_in sockaddress4;

  /* input checking */
  /* set IPv4 port and address */
  sockaddress4.sin_family = AF_INET;
  sockaddress4.sin_port = htons(in_port);
  if (inet_pton(AF_INET, in_ipv4_addr, &(sockaddress4.sin_addr) ) != 1) {
    fprintf(stderr, "sckt4_listen: inet_pton() error\n");
    return(-1);
  }

  /* establish a socket */
  filedesc4 = socket(PF_INET, SOCK_STREAM, 0);
  if (filedesc4 == -1) {
    fprintf(stderr, "sckt4_listen: socket() error\n");
    return(-1);
  }

  /* make non-blocking */
  fcntl_flags = fcntl(filedesc4, F_GETFL, 0); /* get current flags */
  if (fcntl(filedesc4, F_SETFL, fcntl_flags | O_NONBLOCK) == -1) {
    fprintf(stderr, "sckt4_listen: fnctl() O_NONBLOCK error\n");
    close(filedesc4);
    return(-1);
  }

  /* bind IPv4 */
  if (bind(filedesc4, (struct sockaddr*)&sockaddress4, sizeof(sockaddress4) ) == -1) {
    fprintf(stderr, "sckt4_listen: bind() error\n");
    close(filedesc4);
    return(-1);
  }

  /* start listening */
  if (listen(filedesc4, BACKLOG) == -1) {
    fprintf(stderr, "sckt4_listen: listen() error\n");
    close(filedesc4);
    return(-1);
  }

  return(filedesc4);
}


/******************/
/* sckt6_listen() */
/******************/
int
sckt6_listen(
 const char in_ipv6_addr[],
 unsigned short in_port)
{
int filedesc6 = 0;
int v6only = 1;
int fcntl_flags = 0;
struct sockaddr_in6 sockaddress6;

  /* input checking */
  /* set IPv6 port and address */
  sockaddress6.sin6_family = AF_INET6;
  sockaddress6.sin6_port = htons(in_port);
  if (inet_pton(AF_INET6, in_ipv6_addr, &(sockaddress6.sin6_addr) ) != 1) {
    fprintf(stderr, "sckt6_listen: inet_pton() error\n");
    return(-1);
  }

  /* establish a socket */
  filedesc6 = socket(PF_INET6, SOCK_STREAM, 0);
  if (filedesc6 == -1) {
    fprintf(stderr, "sckt6_listen: socket() error\n");
    return(-1);
  }

  /* set IPv6 only - not IPv4 map */
  if (setsockopt(filedesc6, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(int)) != 0) {
    fprintf(stderr, "skct6_listen: setsockopt() error IPV6_V6ONLY\n");
    close(filedesc6);
    return(-1);
  }

  /* make non-blocking */
  fcntl_flags = fcntl(filedesc6, F_GETFL, 0); /* get current flags */
  if (fcntl(filedesc6, F_SETFL, fcntl_flags | O_NONBLOCK) == -1) {
    fprintf(stderr, "sckt6_listen: fnctl() O_NONBLOCK error\n");
    close(filedesc6);
    return(-1);
  }

  /* bind IPv6 */
  if (bind(filedesc6, (struct sockaddr*)&sockaddress6, sizeof(sockaddress6) ) == -1) {
    fprintf(stderr, "sckt6_listen: bind() error\n");
    close(filedesc6);
    return(-1);
  }

  /* start listening */
  if (listen(filedesc6, BACKLOG) == -1) {
    fprintf(stderr, "sckt6_listen: listen() error\n");
    close(filedesc6);
    return(-1);
  }

  return(filedesc6);
}


/*****************/
/* sckt_accept() */
/*****************/
int
sckt_accept(
 int in_fd)
{
struct sockaddr_storage sas;
socklen_t sl_size = 0;
int acpt_fd = 0;

  sl_size = sizeof(struct sockaddr_storage);
  acpt_fd = accept(in_fd, (struct sockaddr*)&sas, &sl_size);
  if (acpt_fd == -1) {
    fprintf(stderr, "sckt_acpt: accept() error\n");
  }
  return(acpt_fd);
}


/***************/
/* sckt_read() */
/***************/
ssize_t
sckt_read(
 int in_fd,
 char *buf,
 size_t size)
{
ssize_t nr = 0;

  nr = read(in_fd, buf, size);
  return(nr);
}


/****************/
/* sckt_write() */
/****************/
ssize_t
sckt_write(
 int in_fd,
 const char *in_buf,
 size_t in_size)
{
ssize_t nw = 0;

  nw = write(in_fd, in_buf, in_size);
  return(nw);
}

/****************/
/* sckt_close() */
/****************/
void
sckt_close(
 int in_fd)
{
  close(in_fd);
}
