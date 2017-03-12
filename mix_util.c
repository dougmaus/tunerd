/* mix_util.c */

/*
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson, with some code and ideas from Chuck Cranor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * modifications:
 *  began with source code from OpenBSD 6.0 mixerctl.c
 *  converted from standalone program to subroutine
 *  and other modifications
 *  2017 Douglas Maus
 */

/* Feature test switches */
/* #define _POSIX_C_SOURCE 200112L */

/* System headers */
/* C language headers */
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <err.h>

/* POSIX headers */
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

/* non POSIX headers */
#include <sys/ioctl.h>
#include <sys/audioio.h>

/* Local headers */
#include "mix_util.h"

/* Macros */
#define FIELD_NAME_MAX  64

#define e_member_name   un.e.member[i].label.name
#define s_member_name   un.s.member[i].label.name

/* File scope variables */
static mixer_devinfo_t *infos = NULL;
static mixer_ctrl_t *values = NULL;

/* External variables */
/* External functions */

/* Structures and unions */
struct field {
 char name[FIELD_NAME_MAX];
 mixer_ctrl_t *valp;
 mixer_devinfo_t *infp;
};
static struct field *fields  = NULL;
static struct field *rfields = NULL;

/* Signal catching functions */


/* Functions */


/* catstr() */
/* out can be same as or overlap p or q */
/*  since catstr() copies into a temporary */
/*  then uses strlcpy to copy into out */
/* (no library error checking) */
static void
catstr(
 const char *p, 
 const char *q, 
 char *out)
{
char tmp[FIELD_NAME_MAX];

  snprintf(tmp, FIELD_NAME_MAX, "%s.%s", p, q);
  strlcpy(out, tmp, FIELD_NAME_MAX);
}


/* findfield() */
/* return pointer or NULL if not found */
static struct field *
findfield(
 const char *name)
{
int i;

  for (i = 0; fields[i].name[0] != '\0'; i++) {
    if (strcmp(fields[i].name, name) == 0) {
      return(&fields[i]);
    }
  }
  return(NULL);
}



/* adjlevel() */
/* return 0 on success, -1 on error */
static int
adjlevel(
 char **p,
 u_char *olevel,
 int more)
{
char *ep, *cp = *p;
long inc;
u_char level;

  if (*cp != '+' && *cp != '-') {
    *olevel = 0;
  }

  errno = 0;
  inc = strtol(cp, &ep, 10);
  if (*cp == '\0' || (*ep != '\0' && *ep != ',') ||
      (errno == ERANGE && (inc == LONG_MAX || inc == LONG_MIN))) {
    fprintf(stderr, "mix_util: adjlevel: error: adjlevel() Bad number %s\n", cp);
    return(-1);
  }

  if (*ep == ',' && !more) {
    fprintf(stderr, "mix_util: adjlevel: error: adjlevel() Too many values\n");
    return(-1);
  }

  *p = ep;

  if (inc < AUDIO_MIN_GAIN - *olevel) {
    level = AUDIO_MIN_GAIN;
  } else if (inc > AUDIO_MAX_GAIN - *olevel) {
    level = AUDIO_MAX_GAIN;
  } else {
    level = *olevel + inc;
  }

  *olevel = level;

  return(0);
}


/* setval() */
/* return 0 on success or non-zero on error */
static int
setval(
 int mix_fd,
 struct field *fieldP,
 char *newvalP)
{
mixer_ctrl_t oldval;
mixer_ctrl_t *m = NULL;
char *s = NULL;
int i, mask;
int status;

  oldval = *fieldP->valp;
  m = fieldP->valp;

  switch (m->type) {
    case AUDIO_MIXER_ENUM:
      if (strcmp(newvalP, "toggle") == 0) {
        for (i = 0; i < fieldP->infp->un.e.num_mem; i++) {
          if (m->un.ord == fieldP->infp->un.e.member[i].ord) {
            break;
          }
        }
        if (i < fieldP->infp->un.e.num_mem) {
          i++;
        } else {
          i = 0;
        }
        m->un.ord = fieldP->infp->un.e.member[i].ord;
        break;
      }
      for (i = 0; i < fieldP->infp->un.e.num_mem; i++) {
        if (strcmp(fieldP->infp->e_member_name, newvalP) == 0) {
          break;
        }
      }
      if (i < fieldP->infp->un.e.num_mem) {
        m->un.ord = fieldP->infp->un.e.member[i].ord;
      } else {
        fprintf(stderr, "mix_util: setval: error: setval() Bad enum value %s\n", newvalP);
        return(-1);
      }
      break;
    case AUDIO_MIXER_SET:
      mask = 0;
      for (; newvalP && *newvalP; newvalP = s) {
        if ((s = strchr(newvalP, ',')) != NULL) {
          *s++ = 0;
        }
        for (i = 0; i < fieldP->infp->un.s.num_mem; i++) {
          if (strcmp(fieldP->infp->s_member_name, newvalP) == 0) {
            break;
          }
        }
        if (i < fieldP->infp->un.s.num_mem) {
          mask |= fieldP->infp->un.s.member[i].mask;
        } else {
          fprintf(stderr, "mix_util: setval: error: setval() Bad set value %s\n", newvalP);
          return(-1);
        }
      }
      m->un.mask = mask;
      break;
    case AUDIO_MIXER_VALUE:
      if (m->un.value.num_channels == 1) {
        status = adjlevel(&newvalP, &m->un.value.level[0], 0);
        if (status != 0) {
          return(status);
        }
      } else {
        status = adjlevel(&newvalP, &m->un.value.level[0], 1);
        if (status != 0) {
          return(status);
        }
        if (*newvalP++ == ',') {
          status = adjlevel(&newvalP, &m->un.value.level[1], 0);
          if (status != 0) {
            return(status);
          }
        } else {
          m->un.value.level[1] = m->un.value.level[0];
        }
      }
      break;
    default:
      fprintf(stderr, "mix_util: setval: error: setval() Invalid format\n");

      return(-1);
  }

  /* set val */
  if (ioctl(mix_fd, AUDIO_MIXER_WRITE, fieldP->valp) < 0) {
    fprintf(stderr, "mix_util: setval: error: ioctl AUDIO_MIXER_WRITE\n");
    return(-1);
  }

  return(0);
}


/************/
/* mixset() */
/************/
/* return: 0 on success, -1 error */
static int
mixset(
 char *mixstr)
{
mixer_devinfo_t dinfo;
struct field *fieldP = NULL;
char *file = NULL;
char *newvalP = NULL;
char *cur_line = NULL;
char *next_line = NULL;
int mix_fd;
int ninfo;
int i, j, pos;
int status = 0;

  if ((file = getenv("MIXERDEVICE")) == 0 || *file == '\0') {
    file = "/dev/mixer";
  }
  if ((mix_fd = open(file, O_RDWR)) == -1) {
    if ((mix_fd = open(file, O_RDONLY)) == -1) {
      fprintf(stderr, "mix_util: mixset: error: unable to open mixer device %s\n", file);
      return(-1);
    }
  }

  /* traverse AUDIO_MIXER_DEVINFO until end, to get number of infos */
  for (ninfo = 0; ; ninfo++) {
    dinfo.index = ninfo;
    if (ioctl(mix_fd, AUDIO_MIXER_DEVINFO, &dinfo) < 0) {
      break;
    }
  }
  if (!ninfo) {
    fprintf(stderr, "mix_util: mixset: error: no mixer devices configured\n");
    return(-1);
  }

  /* allocate memory for, in particular infos */
  if ((infos   = calloc(ninfo, sizeof *infos))   == NULL ||
      (values  = calloc(ninfo, sizeof *values))  == NULL ||
      (rfields = calloc(ninfo, sizeof *rfields)) == NULL ||
      (fields  = calloc(ninfo, sizeof *fields))  == NULL) {
    fprintf(stderr, "mix_util: mixset: error: memory allocation calloc()\n");
    return(-1);
  }

  /* populate infos[i].index starting from 0, then call ioctl to fill each infos */
  for (i = 0; i < ninfo; i++) {
    infos[i].index = i;
    if (ioctl(mix_fd, AUDIO_MIXER_DEVINFO, &infos[i]) < 0) {
      ninfo--;
      i--;
      continue;
    }
  }

  /* prepare rfields[] array */
  for (i = 0; i < ninfo; i++) {
    /* point rfields[i].infp to infos[i] field by ioctl() call */
    rfields[i].infp = &infos[i];

    /* copy infos[i].label.name into rfields[i].name */
    strlcpy(rfields[i].name, infos[i].label.name, FIELD_NAME_MAX);

    /* setup so that rfields[i].valP points to allocated space to store value */
    rfields[i].valp = &values[i];
  }

  /* prepare mixer_ctrl_t, then call ioctl AUDIO_MIXER_READ to get values[] */
  for (i = 0; i < ninfo; i++) {
    values[i].dev = i;
    values[i].type = infos[i].type;

    if (infos[i].type != AUDIO_MIXER_CLASS) {
      /* try as stereo (num_channels = 2) first, then try mono, then fail */
      values[i].un.value.num_channels = 2;
      if (ioctl(mix_fd, AUDIO_MIXER_READ, &values[i]) < 0) {
        values[i].un.value.num_channels = 1;
        if (ioctl(mix_fd, AUDIO_MIXER_READ, &values[i]) < 0) {
          /* unrecoverable */
          fprintf(stderr, "mix_util: mixset: error: ioctl AUDIO_MIXER_READ\n");
          free(infos);
          free(values);
          free(rfields);
          free(fields);
          return(-1);
        }
      }
    }
  }

  for (j = i = 0; i < ninfo; i++) {
    if (infos[i].type != AUDIO_MIXER_CLASS &&
        infos[i].prev == AUDIO_MIXER_LAST) {
      fields[j++] = rfields[i]; /* copy the struct */
      for (pos = infos[i].next; pos != AUDIO_MIXER_LAST;
           pos = infos[pos].next) {
        fields[j] = rfields[pos];
        catstr(rfields[i].name, infos[pos].label.name, fields[j].name);
        j++;
      }
    }
  }

  for (i = 0; i < j; i++) {
    int cls = fields[i].infp->mixer_class;
    if (cls >= 0 && cls < ninfo) {
      catstr(infos[cls].label.name, fields[i].name, fields[i].name);
    }
  }

  /* apply a new setting */
  cur_line = mixstr;
  do {
    next_line = strchr(cur_line, '\n');
    if (next_line != NULL) {
      *next_line = '\0';
      next_line++;
    }

    newvalP = strchr(cur_line, '=');
    /* should do some checking that '=' wasn't end of string */
    if (newvalP != NULL) {
      *newvalP = '\0';
      newvalP += 1;
    }
    fieldP = findfield(cur_line);
    if (fieldP != NULL) {
      status = setval(mix_fd, fieldP, newvalP);
    } else {
      fprintf(stderr, "mix_util: mixset: error: field %s does not exist\n", cur_line);
      status = -1;
    }
    cur_line = next_line;
  } while (next_line != NULL);

  /* cleanup */
  free(infos);
  free(values);
  free(rfields);
  free(fields);

  return(status);
}


/**************/
/* mix_init() */
/**************/
/* initialize mixer */
/*  settings for either radio tuner or files */
/*  defaults to source = line */
/* return: 0 on success, -1 error */
int
mix_init(void)
{
char mix_ini_str[] = 
 "outputs.master=255\n"
 "inputs.mix_line=255\n"
 "inputs.mix_line-in=255\n"
 "inputs.mix_source=line-in";
int status = 0;

  status = mixset(mix_ini_str);
  return(status);
}


/***************/
/* mix_radio() */
/***************/
/*  set mixer to radio tuner (line-in) */
/* return: 0 on success, -1 error */
int
mix_radio(void)
{
char mix_radio_str[] = "inputs.mix_source=line-in";
int status = 0;

  status = mixset(mix_radio_str);
  return(status);
}


/***************/
/* mix_files() */
/***************/
/*  set mixer to files (WAV/MP3,etc = line) */
/* return: 0 on success, -1 error */
int
mix_files(void)
{
char mix_files_str[] = "inputs.mix_source=line";
int status = 0;

  status = mixset(mix_files_str);
  return(status);
}
