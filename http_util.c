/* http_util.c */

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

/* Local headers */
#include "http_util.h"
#include "sckt_util.h"

/* Macros */
#ifndef ROOTHTMLPATH
#define ROOTHTMLPATH "/var/tunerd/root.html"
#endif
#define MAXURISIZE 8192

/* File scope variables */
static char *root_resp = NULL;
static int root_size = 0;

/* External variables */
/* External functions */

/* Structures and unions */
struct cb_struct {
  int method;
  char path_match[MAXURISIZE+1];
  int (*f)(const char*, int);
};
static struct cb_struct cb[16];
static int cb_count = 0;

enum HTTP_methods {
 OPTIONS = 1,
 GET = 2,
 HEAD = 3,
 POST = 4,
 PUT = 5,
 DELETE = 6,
 TRACE = 7,
 CONNECT = 8
};

/* Signal catching functions */


/* Functions */


/***************/
/* http_init() */
/***************/
/* return: 0 on success, -1 error */
int
http_init(void)
{
FILE *fp = NULL;
char resp_head[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
char content_len_str[8];
char *p = NULL;
int head_len = 0;
long file_size = 0;
size_t nread = 0;

  /* load the root HTML document into memory, prepended with HTTP header */

  /* length of HTML */
  fp = fopen(ROOTHTMLPATH, "r");
  if (fp == NULL) {
    fprintf(stderr, "http_init: fopen() error %s\n", ROOTHTMLPATH);
    return(-1);
  }
  fseek(fp, 0L, SEEK_END);
  file_size = ftell(fp);

  head_len = strlen(resp_head); /* known at compile time */
  head_len += snprintf(content_len_str, 8, "%ld", file_size);
  head_len += 4; /* /r/n/r/n */

  root_size = head_len + file_size;

  /* malloc enough memory */ 
  root_resp = malloc(sizeof(char) * root_size );
  if (root_resp == NULL) {
    fprintf(stderr, "http_init: malloc() error root HTML\n");
    fclose(fp);
    return(-1);
  }
  root_resp[0] = '\0';

  /* write into memory */
  /*  HTTP header */
  strcat(root_resp, resp_head);
  strcat(root_resp, content_len_str);
  strcat(root_resp, "\r\n\r\n");

  /*  HTML document */
  p = &(root_resp[head_len]);
  fseek(fp, 0L, SEEK_SET);
  nread = fread(p, sizeof(char), file_size, fp);
  if (nread != file_size) {
    fprintf(stderr, "http_init: fread() error %s\n", ROOTHTMLPATH);
  }

  fclose(fp);

  if (nread != file_size) return(-1);
  return(0);
}


/*******************/
/* http_callback() */
/*******************/
/* register callback */
/* in_method and in_path_match MUST BE NULL TERMINATED BY CALLER */
/*  or else badness */
/* return: 0 on success, -1 error */
int
http_callback(
 const char *in_method,
 const char *in_path_match,
 int (*in_f)(const char *, int))
{
size_t path_len = 0;

  if (strncmp(in_method, "GET", 3) == 0) cb[cb_count].method = GET;
  else if (strncmp(in_method, "POST", 4) == 0) cb[cb_count].method = POST;
  else if (strncmp(in_method, "HEAD", 4) == 0) cb[cb_count].method = HEAD;
  else {
    cb[cb_count].method = 0;
    return(-1);
  }

  path_len = strlen(in_path_match);
  if (path_len > MAXURISIZE) {
    fprintf(stderr, "http_callback: warning: callback path exceeds maximum URI size\n");
    path_len = MAXURISIZE; 
  }

  strncpy(cb[cb_count].path_match, in_path_match, path_len);
  (cb[cb_count].path_match)[path_len] = '\0';
  cb[cb_count].f = in_f;
  cb_count += 1;

  return(0);
}


/***************/
/* http_root() */
/***************/
/* return: -1 keep alive socket */
/*          0 close socket */
int
http_root(
 const char *in_req,
 int in_fd)
{

  sckt_write(in_fd, root_resp, root_size);

  return(0);
}


/**************/
/* http_403() */
/**************/
/* return 0 close socket */
int
http_403(
 const char *in_req,
 int in_fd)
{
char resp[] = "HTTP/1.1 403 Forbidden\r\nContent-length: 0\r\nConnection: close\r\n\r\n";

  sckt_write(in_fd, resp, strlen(resp));
   /* strlen() of string literal should be constant at compile time */

  return(0);
}


/**************/
/* http_404() */
/**************/
/* return 0 close socket */
int
http_404(
 const char *in_req,
 int in_fd)
{
char resp[] = "HTTP/1.1 404 Not Found\r\nContent-length: 0\r\nConnection: close\r\n\r\n";

  sckt_write(in_fd, resp, strlen(resp));
   /* strlen() of string literal should be constant at compile time */

  return(0);
}


/*****************/
/* http_handle() */
/*****************/
/* caller IS EXPECTED TO HAVE in_req BE NULL TERMINATED */
/* return: */
/*  -1 for keep alive socket */
/*   0 for close socket */
int
http_handle(
 const char *in_req,
 int in_fd)
{
char path[MAXURISIZE+1];
size_t in_req_len = 0;
int method_code = 0;
int path_len = 0;
int i = 0;
int close_flag = 0;
int p1 = 0;
int p2 = 0;

  /* wait until HTTP request header complete */
  /*  return to event loop, keep connection open */
  if (strstr(in_req, "\r\n\r\n") == NULL) {
    return(-1);
  }

  in_req_len = strlen(in_req);

  /* get method */
  if (strncmp(in_req, "GET ", 4) == 0) method_code = GET;
  else if (strncmp(in_req, "POST ", 5) == 0) method_code = POST;
  else if (strncmp(in_req, "HEAD ", 5) == 0) method_code = HEAD;
  else {
    fprintf(stderr, "http_handle: HTTP request method invalid\n");
    http_403(in_req, in_fd);
    return(0);
  }

  /* find delimits of Request-URI */

  /* skip over method to find start of Request-URI */
  /*  by above, guaranteed to have a space after GET or POST or HEAD */
  p1 = 0;
  while (in_req[p1] != ' ') p1++;
  while (in_req[p1] == ' ') p1++;

  /* find end of Request-URI */
  /*  from now on out, need to make sure don't exceed buffer */
  p2 = p1;
  while ((in_req[p2] != ' ')  && (p2 < in_req_len)) p2++;
  if (p2 == in_req_len) {
    fprintf(stderr, "http_handle: HTTP request URI invalid - no space after URI\n");
    http_403(in_req, in_fd);
    return(0);
  }

  /* need path, skip transport://host:port */
  if (strncmp(&in_req[p1], "http://", 7) == 0) {
    /* is an absoluteURI, so parse to get path(and params) */
    p1 += 7;
    while ((in_req[p1] != '/') && (p1 < p2)) p1++;
    if (p1 == p2) {
      fprintf(stderr, "http_handle: HTTP request URI invalid - incomplete absolute path\n");
      http_403(in_req, in_fd);
      return(0);
    }
  }

  /* p1 is first char of path, p2 is first space after path */
  path_len = p2 - p1;
  if (path_len <= MAXURISIZE) { 
    strncpy(path, &(in_req[p1]), path_len);
    path[path_len] = '\0';
  } else {
    fprintf(stderr, "http_handle: HTTP request path exceeds %d\n", MAXURISIZE);
    http_403(in_req, in_fd);
    return(0);
  }

  if ((method_code == GET) && (strncmp(path, "/", 2) == 0)) {
    /* respond with root HTML and return 0 to close socket */
    http_root(in_req, in_fd);
    return(0);
  }

  for (i = 0; i < cb_count; i++) {
    if (method_code == cb[i].method) {
      /* include checking null character at end of path, for exact match */
      if (strncmp(cb[i].path_match, path, path_len+1) == 0) {
        close_flag = cb[i].f(in_req, in_fd);
        return(close_flag);
      }
    }
  }

  /* not found, error and return 0 to close socket */
  fprintf(stderr, "http_handle: HTTP request URI not found, %s\n", path);
  http_404(in_req, in_fd);
  return(0);
}
