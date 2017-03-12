/* presets.c */

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
#include "presets.h"

/* Macros */
#ifndef PRESETSPATH
#define PRESETSPATH "/var/tunerd/presets.txt"
#endif

/* File scope variables */
static long *preset = NULL;
unsigned short preset_size = 0;
unsigned short preset_count = 0;
short preset_cur = 0;

/* External variables */
/* External functions */
/* Structures and unions */
/* Signal catching functions */


/* Functions */


/**********/
/* read() */
/**********/
/* return 0 on success, -1 on error */
static int
presets_read(void)
{
FILE *fp = NULL;
char line[256];
int i;

  fp = fopen(PRESETSPATH, "r");
  if (fp == NULL) {
    fprintf(stderr, "presets_read: fopen() error %s\n", PRESETSPATH);
    return(-1);
  }

  line[0] = '\0';
  i = 0;
  while (fgets(line, 255, fp) != NULL) {
    if (i >= preset_size) {
      preset_size *= 2;
      preset = (long*) realloc(preset, sizeof(long) * preset_size);
      if (preset == NULL) {
        fprintf(stderr, "presets_read: realloc() error\n");
        return(-1);
      }
    }
    if (sscanf(line, "%ld", &preset[i]) > 0) {
      preset_count += 1;
      i += 1;
    }
  }
  if (ferror(fp)) {
    fprintf(stderr, "presets_read: fgets() error presets.txt\n");
    return(-1);
  }

  fclose(fp);

  return(0);
}


/***********/
/* write() */
/***********/
/* return 0 on success, -1 on error */
static int
presets_write(void)
{
FILE *fp = NULL;
unsigned short i;

  fp = fopen(PRESETSPATH, "w");
  if (fp == NULL) {
    fprintf(stderr, "presets_write: fopen() error %s\n", PRESETSPATH);
    return(-1);
  }

  for (i = 0; i < preset_count; i++) {
    fprintf(fp, "%ld\n", preset[i]);
  }

  if (ferror(fp)) {
    fprintf(stderr, "presets_write: fprintf() error presets.txt\n");
    return(-1);
  }

  fclose(fp);

  return(0);
}


/******************/
/* presets_init() */
/******************/
/* return 0 on success, -1 on error */
int
presets_init(void)
{
  if (preset != NULL) {
    fprintf(stderr, "presets_init: repeat call\n");
    return(-1);
  }

  preset = (long*) malloc(sizeof(long) * 16);
  preset_size = 16;
  preset_count = 0;

  presets_read();

  preset_cur = -1;

  return(0);
}


/*****************/
/* presets_end() */
/*****************/
void
presets_end(void)
{
  if (preset != NULL) free(preset);
}


/******************/
/* presets_next() */
/******************/
/* return: next preset value, -1 on error */
long
presets_next(void)
{
  /* input checking */
  if (preset_count == 0) return(-1);

  preset_cur += 1;
  if (preset_cur >= preset_count) {
    preset_cur = 0;
  }

  return(preset[preset_cur]);
}


/********************/
/* presets_insert() */
/********************/
/* insert a preset before preset_cur */
/* return 0 on success, -1 on error */
int
presets_insert(long new_preset)
{
short i;

  /* expand array if necessary */
  if (preset_count >= preset_size) {
    preset_size *= 2;
    preset = (long*) realloc(preset, sizeof(long) * preset_size);
    if (preset == NULL) {
      fprintf(stderr, "presets_insert: realloc() error\n");
      return(-1);
    }
  }

  if (preset_cur < 0) preset_cur = 0;

  /* cascade up */
  for (i = preset_count; i > preset_cur; i--) {
    preset[i] = preset[i-1];
  }
  preset_count += 1;
  preset[preset_cur] = new_preset;

  presets_write();

  return(0);
}


/********************/
/* presets_delete() */
/********************/
/* delete preset that is preset_cur */
/* return 0 on success, -1 on error */
int
presets_delete(void)
{
short i;

  /* if no current preset, return */
  if (preset_cur == -1) return(-1);

  /* cascade down */
  for (i = preset_cur; i < (preset_count-1); i++) {
    preset[i] = preset[i+1];
  }
  preset[preset_count] = 0;
  preset_count -= 1;

  presets_write();

  return(0);
}
