/* main.c */

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
#include <time.h>

/* POSIX headers */
/*  POSIX Issue 1 */
#include <unistd.h> /* fork, setsid, chdir */
#include <fcntl.h>

/* Local headers */
#include "sckt_util.h"
#include "evnt_util.h"
#include "http_util.h"
#include "sse_util.h"

#include "tunerd.h"

/* Macros */
#ifndef NULLPATH
#define NULLPATH "/dev/null"
#endif

#ifndef LOGPATH
#define LOGPATH "/var/tunerd/tunerd.log"
#endif

#ifndef MAXCONNECTIONS
#define MAXCONNECTIONS 32 
#endif

/* File scope variables */
/* External variables */
/* External functions */
/* Structures and unions */
/* Signal catching functions */


/* Functions */


/********************/
/* filltimestring() */
/********************/
static void
filltimestring(
 char *string)
{
struct tm *time_struct;
time_t time_val;

  time(&time_val);
  time_struct = gmtime(&time_val);
  strftime(string, 32, "%Y-%m-%d %H:%M:%S UTC ", time_struct);

}


/**********/
/* init() */
/**********/
/* return: 0 success, -1 error */
static int
init(
 int *io_fd4,
 int *io_fd6)
{
int status = 0;
int t4 = -1;
int t6 = -1;

  /* set up sockets for listening */
  t4 = sckt4_listen("0.0.0.0", 80);
  t6 = sckt6_listen("::0", 80);

  /* initialize eventloop functions */
  status = evnt_init(t4, t6, MAXCONNECTIONS);
  if (status == (-1)) {
    return(status);
  }

  /* return file descriptors back to calling function */
  *io_fd4 = t4;
  *io_fd6 = t6;

  status = http_init();
  if (status == (-1)) {
    return(status);
  }

  status = sse_init();
  if (status == (-1)) {
    return(status);
  }

  status = tunerd_init();

  return(status);
}


/*********/
/* end() */
/*********/
static void
end(
 int in_fd4,
 int in_fd6)
{
  evnt_end();

  if (in_fd4 >= 0) sckt_close(in_fd4);
  if (in_fd6 >= 0) sckt_close(in_fd6);
}


/**********/
/* main() */
/**********/
int
main(
 int argc,
 char *argv[])
{
FILE *fp = NULL; /* for stderr = log file */
pid_t pid = 0;
char timestamp[32];
int fd = 0;
int status = 0;
int fd4 = 0;
int fd6 = 0;

  /* daemon */
  /* fork */
  pid = fork();
  if (pid == (-1)) {
    perror("main: fork() error");
    return(EXIT_FAILURE);
  } else if (pid != 0) {
    /* parent, not exit but _exit to avoid closing child streams */
    _exit(EXIT_SUCCESS);
  }

  /* setsid */
  if (setsid() == (-1)) {
    perror("main: setsid() error");
    return(EXIT_FAILURE);
  }

  /* change working directory to "/" */
  if (chdir("/") == -1) {
    perror("main: chdir() error");
  }

  /* set stdin and stdout to /dev/null */
  fd = open(NULLPATH, O_RDWR, 0);
  if (fd == (-1)) {
    perror("main: open() error null device");
    return(EXIT_FAILURE);
  }
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  if (fd > 1) close(fd);

  /* set stderr to a log file (buffered) */
  fp = fopen(LOGPATH, "a");
  if (fp == NULL) {
    perror("main: fopen() error for log");
    return(EXIT_FAILURE);
  }
  if (dup2(fileno(fp), STDERR_FILENO) == -1) {
    perror("main: dup2() error stderr");
    return(EXIT_FAILURE);
  }

  filltimestring(timestamp);
  fprintf(stderr, "%s tunerd: starting up\n", timestamp);


  /* main program */
  status = init(&fd4, &fd6);

  if (status == 0) {
    status = evnt_loop();
  }

  end(fd4, fd6);

  filltimestring(timestamp);
  fprintf(stderr, "%s tunerd: shutting down\n", timestamp);

  fclose(fp);

  return(EXIT_SUCCESS);
}
