
CC = cc
CFLAGS = -std=c99 -pedantic -Wall
LDFLAGS = -lm

tunerd : main.c sckt_util.h sckt_util.c evnt_util.h evnt_util.c http_util.h http_util.c sse_util.h sse_util.c presets.h presets.c mix_util.h mix_util.c radio_util.h radio_util.c tunerd.h tunerd.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ main.c sckt_util.c evnt_util.c http_util.c sse_util.c presets.c mix_util.c radio_util.c tunerd.c

