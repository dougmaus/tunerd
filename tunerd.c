/* tunerd.c */

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
#include "sckt_util.h"
#include "http_util.h"
#include "sse_util.h"
#include "mix_util.h"
#include "radio_util.h"
#include "presets.h"

/* Macros */
#define MAXSSE 32

/* File scope variables */
static long radio_freq = 0;

static int sse_desc_freq;

/* External variables */
/* External functions */
/* Structures and unions */
/* Signal catching functions */


/* Functions */


/**************/
/* get_freq() */
/**************/
/* handles HTTP request GET freq */
/*  with Server Sent Events (SSE) */
/* as a HTTP callback function: */
/*  return:  0 for close socket */
/*          -1 keep alive socket */
int
get_freq(
 const char *in_req,
 int in_fd)
{
char message[128];
int status = 0;

  /* add socket to SSE listeners */
  if (sse_desc_freq == (-1)) {
    sse_desc_freq = sse_new(in_fd);
    if (sse_desc_freq == (-1)) {
      fprintf(stderr, "get_freq: error in sse_new\n");
      return(0);
    }
  } else {
    status = sse_add(sse_desc_freq, in_fd);
    if (status == (-1)) {
      fprintf(stderr, "get_freq: error in sse_add\n");
      return(0);
    }
  }

  /* send HTTP header and data (reference/standard for text/event-stream allows single LF) */
  snprintf(message, 128, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/event-stream\r\n\r\ndata: %ld\n\n", radio_freq);
  sckt_write(in_fd, message, strlen(message));

  /* return with code to keep socket alive (-1) */
  return(-1);
}


/*****************/
/* post_preset() */
/*****************/
/* as a HTTP callback function: */
/*  return:  0 for close socket */
/*          -1 keep alive socket */
int
post_preset(
 const char *in_req,
 int in_fd)
{
char HTTP_resp[] = "HTTP/1.1 204 No Content\r\nConnection: close\r\n\r\n";
char data_message[64];

  /* get next preset */
  radio_freq = presets_next();

  /* set the radio device frequency */
  radio_frequency(radio_freq);

  /* send updated frequency to all SSE listeners */
  snprintf(data_message, 64, "data: %ld\n\n", radio_freq);
  sse_send(sse_desc_freq, data_message, 0);

  /* send a valid response to this POST connection */
  sckt_write(in_fd, HTTP_resp, strlen(HTTP_resp));

  /* close socket, by returning 0 */
  return(0);
}


/*****************/
/* tunerd_init() */
/*****************/
/* return: 0 on success, -1 error */
int
tunerd_init(void)
{

  /* initialize mixerctl settings */
  mix_init();
  mix_radio(); /* set to radio mode */

  /* initalize radioctl settings */
  radio_init();
  radio_freq = 99500;
  radio_frequency(radio_freq);

  presets_init();

  /* no SSE listeners yet */
  sse_desc_freq = (-1);

  /* set HTTP callbacks */
  http_callback("GET", "/radio_freq", get_freq);
  http_callback("POST", "/radio_preset", post_preset);

  return(0);
}
