/* sckt_util.h */

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

#ifndef sckt_util_h
#define sckt_util_h

#include <sys/types.h>
#include <unistd.h>

/* for sckt_listen functions, the in_ipvN_addr is passed to inet_pton */
/*  valid forms vary by architecture */
/* typical: "127.0.0.1" and "0.0.0.0" work for IPv4 */
/*  and "::1" and "::0" work for IPv6 */

int sckt4_listen(const char in_ipv4_addr[], unsigned short in_port);

int sckt6_listen(const char in_ipv6_addr[], unsigned short in_port);

int sckt_accept(int in_fd);

ssize_t sckt_read(int in_fd, char *buf, size_t size);

ssize_t sckt_write(int in_fd, const char *buf, size_t size);

void sckt_close(int in_fd);

#endif
