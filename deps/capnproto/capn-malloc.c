/* vim: set sw=8 ts=8 sts=8 noet: */
/* capn-malloc.c
 *
 * Copyright (C) 2013 James McKaskill
 * Copyright (C) 2014 Steve Dee
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "capnp_c.h"
#include "capnp_priv.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/*
 * 8 byte alignment is required for struct capn_segment.
 * This struct check_segment_alignment verifies this at compile time.
 *
 * Unless capn_segment is defined with 8 byte alignment, check_segment_alignment
 * fails to compile in x86 mode (or on another CPU with 32-bit pointers),
 * as (sizeof(struct capn_segment)&7) -> (44 & 7) evaluates to 4.
 * It compiles in x64 mode (or on another CPU with 64-bit pointers),
 * as (sizeof(struct capn_segment)&7) -> (80 & 7) evaluates to 0.
 */
struct check_segment_alignment {
	unsigned int foo : (sizeof(struct capn_segment)&7) ? -1 : 1;
};

static struct capn_segment *create(void *u, uint32_t id, int sz) {
	struct capn_segment *s;
	sz += sizeof(*s);
	if (sz < 4096) {
		sz = 4096;
	} else {
		sz = (sz + 4095) & ~4095;
	}
	s = (struct capn_segment*) calloc(1, sz);
	s->data = (char*) (s+1);
	s->cap = sz - sizeof(*s);
	s->user = s;
	return s;
}

static struct capn_segment *create_local(void *u, int sz) {
	return create(u, 0, sz);
}

void capn_init_malloc(struct capn *c) {
	memset(c, 0, sizeof(*c));
	c->create = &create;
	c->create_local = &create_local;
}

void capn_free(struct capn *c) {
	struct capn_segment *s = c->seglist;
	while (s != NULL) {
		struct capn_segment *n = s->next;
		free(s->user);
		s = n;
	}
	capn_reset_copy(c);
}

void capn_reset_copy(struct capn *c) {
	struct capn_segment *s = c->copylist;
	while (s != NULL) {
		struct capn_segment *n = s->next;
		free(s->user);
		s = n;
	}
	c->copy = NULL;
	c->copylist = NULL;
}

#define ZBUF_SZ 4096

static int read_fp(void *p, size_t sz, FILE *f, struct capn_stream *z, uint8_t* zbuf, int packed) {
	if (f && packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;

		while (z->avail_out && capn_inflate(z) == CAPN_NEED_MORE) {
			int r;
			memmove(zbuf, z->next_in, z->avail_in);
			r = fread(zbuf+z->avail_in, 1, ZBUF_SZ - z->avail_in, f);
			if (r <= 0)
				return -1;
			z->avail_in += r;
		}
		return 0;

	} else if (f && !packed) {
		return fread(p, sz, 1, f) != 1;

	} else if (packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;
		return capn_inflate(z) != 0;

	} else {
		if (z->avail_in < sz)
			return -1;
		memcpy(p, z->next_in, sz);
		z->next_in += sz;
		z->avail_in -= sz;
		return 0;
	}
}

static int init_fp(struct capn *c, FILE *f, struct capn_stream *z, int packed) {
	/*
	 * Initialize 'c' from the contents of 'f', assuming the message has been
	 * serialized with the standard framing format. From https://capnproto.org/encoding.html:
	 *
	 * When transmitting over a stream, the following should be sent. All integers are unsigned and little-endian.
	 *   (4 bytes) The number of segments, minus one (since there is always at least one segment).
	 *   (N * 4 bytes) The size of each segment, in words.
	 *   (0 or 4 bytes) Padding up to the next word boundary.
	 *   The content of each segment, in order.
	 */

	struct capn_segment *s = NULL;
	uint32_t i, segnum, total = 0;
	uint32_t hdr[1024];
	uint8_t zbuf[ZBUF_SZ];
	char *data = NULL;

	capn_init_malloc(c);

	/* Read the first four bytes to know how many headers we have */
	if (read_fp(&segnum, 4, f, z, zbuf, packed))
		goto err;

	segnum = capn_flip32(segnum);
	if (segnum > 1023)
		goto err;
	segnum++; /* The wire encoding was zero-based */

	/* Read the header list */
	if (read_fp(hdr, 8 * (segnum/2) + 4, f, z, zbuf, packed))
		goto err;

	for (i = 0; i < segnum; i++) {
		uint32_t n = capn_flip32(hdr[i]);
		if (n > INT_MAX/8 || n > UINT32_MAX/8 || UINT32_MAX - total < n*8)
			goto err;
		hdr[i] = n*8;
		total += hdr[i];
	}

	/* Allocate space for the data and the capn_segment structs */
	s = (struct capn_segment*) calloc(1, total + (sizeof(*s) * segnum));
	if (!s)
		goto err;

	/* Now read the data and setup the capn_segment structs */
	data = (char*) (s+segnum);
	if (read_fp(data, total, f, z, zbuf, packed))
		goto err;

	for (i = 0; i < segnum; i++) {
		s[i].len = s[i].cap = hdr[i];
		s[i].data = data;
		data += s[i].len;
		capn_append_segment(c, &s[i]);
	}

    /* Set the entire region to be freed on the last segment */
	s[segnum-1].user = s;

	return 0;

err:
	memset(c, 0, sizeof(*c));
	free(s);
	return -1;
}

int capn_init_fp(struct capn *c, FILE *f, int packed) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	return init_fp(c, f, &z, packed);
}

int capn_init_mem(struct capn *c, const uint8_t *p, size_t sz, int packed) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	z.next_in = p;
	z.avail_in = sz;
	return init_fp(c, NULL, &z, packed);
}

static void header_calc(struct capn *c, uint32_t *headerlen, size_t *headersz)
{
	/* segnum == 1:
	 *   [segnum][segsiz]
	 * segnum == 2:
	 *   [segnum][segsiz][segsiz][zeroes]
	 * segnum == 3:
	 *   [segnum][segsiz][segsiz][segsiz]
	 * segnum == 4:
	 *   [segnum][segsiz][segsiz][segsiz][segsiz][zeroes]
	 */
	*headerlen = ((2 + c->segnum) / 2) * 2;
	*headersz = 4 * *headerlen;
}

static int header_render(struct capn *c, struct capn_segment *seg, uint32_t *header, uint32_t headerlen, size_t *datasz)
{
	size_t i;

	header[0] = capn_flip32(c->segnum - 1);
	header[headerlen-1] = 0; /* Zero out the spare position in the header sizes */
	for (i = 0; i < c->segnum; i++, seg = seg->next) {
		if (0 == seg)
			return -1;
		*datasz += seg->len;
		header[1 + i] = capn_flip32(seg->len / 8);
	}
	if (0 != seg)
		return -1;

	return 0;
}

static int capn_write_mem_packed(struct capn *c, uint8_t *p, size_t sz)
{
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint32_t *header;
	struct capn_stream z;
	int ret;

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);
	header = (uint32_t*) (p + headersz + 2); /* must reserve two bytes for worst case expansion */

	if (sz < headersz*2 + 2) /* We must have space for temporary writing of header to deflate */
		return -1;

	ret = header_render(c, root.seg, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	memset(&z, 0, sizeof(z));
	z.next_in = (uint8_t *)header;
	z.avail_in = headersz;
	z.next_out = p;
	z.avail_out = sz;

	// pack the headers
	ret = capn_deflate(&z);
	if (ret != 0 || z.avail_in != 0)
		return -1;

	for (seg = root.seg; seg; seg = seg->next) {
		z.next_in = (uint8_t *)seg->data;
		z.avail_in = seg->len;
		ret = capn_deflate(&z);
		if (ret != 0 || z.avail_in != 0)
			return -1;
	}

	return sz - z.avail_out;
}

int
capn_write_mem(struct capn *c, uint8_t *p, size_t sz, int packed)
{
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint32_t *header;
	int ret;

	if (c->segnum == 0)
		return -1;

	if (packed)
		return capn_write_mem_packed(c, p, sz);

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);
	header = (uint32_t*) p;

	if (sz < headersz)
		return -1;

	ret = header_render(c, root.seg, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (sz < headersz + datasz)
		return -1;

	p += headersz;

	for (seg = root.seg; seg; seg = seg->next) {
		memcpy(p, seg->data, seg->len);
		p += seg->len;
	}

	return headersz+datasz;
}

static int _write_fd(ssize_t (*write_fd)(int fd, const void *p, size_t count), int fd, void *p, size_t count)
{
	ssize_t ret;
	size_t sent = 0;

	while (sent < count) {
		ret = write_fd(fd, ((uint8_t*)p)+sent, count-sent);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
				return -1;
		}
		sent += ret;
	}

	return 0;
}

int capn_write_fd(struct capn *c, ssize_t (*write_fd)(int fd, const void *p, size_t count), int fd, int packed)
{
	unsigned char buf[4096];
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	int ret;
	struct capn_stream z;
	unsigned char *p;

	if (c->segnum == 0)
		return -1;

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);

	if (sizeof(buf) < headersz)
		return -1;

	ret = header_render(c, root.seg, (uint32_t*)buf, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (packed) {
		const int headerrem = sizeof(buf) - headersz;
		const int maxpack = headersz + 2;
		if (headerrem < maxpack)
			return -1;

		memset(&z, 0, sizeof(z));
		z.next_in = buf;
		z.avail_in = headersz;
		z.next_out = buf + headersz;
		z.avail_out = headerrem;
		ret = capn_deflate(&z);
		if (ret != 0)
			return -1;

		p = buf + headersz;
		headersz = headerrem - z.avail_out;
	} else {
		p = buf;
	}

	ret = _write_fd(write_fd, fd, p, headersz);
	if (ret < 0)
		return -1;

	datasz = headersz;
	for (seg = root.seg; seg; seg = seg->next) {
		size_t bufsz;
		if (packed) {
			memset(&z, 0, sizeof(z));
			z.next_in = (uint8_t*)seg->data;
			z.avail_in = seg->len;
			z.next_out = buf;
			z.avail_out = sizeof(buf);
			ret = capn_deflate(&z);
			if (ret != 0)
				return -1;
			p = buf;
			bufsz = sizeof(buf) - z.avail_out;
		} else {
			p = (uint8_t*)seg->data;
			bufsz = seg->len;
		}
		ret = _write_fd(write_fd, fd, p, bufsz);
		if (ret < 0)
			return -1;
		datasz += bufsz;
	}

	return datasz;
}
