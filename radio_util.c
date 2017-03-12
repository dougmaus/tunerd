/* radio_util.c */

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
#include <math.h> /* lrint() */

/* POSIX headers */
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

/* non POSIX headers */
#include <sys/ioctl.h>
#include <sys/radioio.h>

/* Local headers */
#include "radio_util.h"

/* Macros */
/* File scope variables */
/* External variables */
/* External functions */
/* Structures and unions */
/* Signal catching functions */

/* Functions */

/****************/
/* radio_init() */
/****************/
/* default to 99.5 MHz */
/*  return 0 on success, -1 on error */
int
radio_init(void)
{
struct radio_info radio_info_struct;
int radio_fd = 0;
int status = 0;

  /* open /dev/radio in read/write mode */
  radio_fd = open("/dev/radio", O_RDWR);
  if (radio_fd < 0) {
    fprintf(stderr, "radio_init: open() error /dev/radio\n");
    return(-1);
  }

  /* first, fill struct radio_info with current values */
  status = ioctl(radio_fd, RIOCGINFO, &radio_info_struct);
  if (status != -1) {
    /* change the desired values, leaving others as they are */
    radio_info_struct.mute = 0; /* 0=false */
    radio_info_struct.tuner_mode = RADIO_TUNER_MODE_RADIO;
    radio_info_struct.freq = 99500; /* kHz */
    status = ioctl(radio_fd, RIOCSINFO, &radio_info_struct);
    if (status == -1) {
      fprintf(stderr, "radio_init: ioctl() error setting radio_info_struct\n");
    }
  } else {
    fprintf(stderr, "radio_init: ioctl() error reading radio_info_struct\n");
  }

  close(radio_fd);

  return(status);
}


/*********************/
/* radio_frequency() */
/*********************/
/* freq as integer in kHz */
/*  return 0 on success, -1 on error */
int
radio_frequency(
 unsigned long in_kHz)
{
struct radio_info radio_info_struct;
int radio_fd = 0;
int status = 0;

unsigned long kHz = in_kHz;

  /* input checking */
  if (kHz < 87500) {
    kHz = 87500;
  } else if (kHz > 108000) {
    kHz = 108000;
  }

  /* open /dev/radio in read/write mode */
  radio_fd = open("/dev/radio", O_RDWR);
  if (radio_fd < 0) {
    fprintf(stderr, "radio_frequency: open() error /dev/radio\n");
    return(-1);
  }

  /* first, fill struct radio_info with current values */
  status = ioctl(radio_fd, RIOCGINFO, &radio_info_struct);
  if (status != -1) {
    /* change the desired values, leaving others as they are */
    radio_info_struct.freq = kHz;
    status = ioctl(radio_fd, RIOCSINFO, &radio_info_struct);
    if (status == -1) {
      fprintf(stderr, "radio_frequency: ioctl() error setting radio_info_struct\n");
    }
  } else {
    fprintf(stderr, "radio_frequency: ioctl() error reading radio_info_struct\n");
  }

  close(radio_fd);

  return(status);
}
