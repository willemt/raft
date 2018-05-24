/* vim: set sw=8 ts=8 sts=8 noet: */
/* capn-stream.c
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "capnp_c.h"
#include "capnp_priv.h"
#include <string.h>

#ifndef min
static unsigned min(unsigned a, unsigned b) { return (a < b) ? a : b; }
#endif

int capn_deflate(struct capn_stream* s) {
	if (s->avail_in % 8) {
		return CAPN_MISALIGNED;
	}

	while (s->avail_in) {
		int i;
		size_t sz;
		uint8_t hdr = 0;
		uint8_t *p;

		if (!s->avail_out)
			return CAPN_NEED_MORE;

		if (s->raw > 0) {
			sz = min(s->raw, min(s->avail_in, s->avail_out));
			memcpy(s->next_out, s->next_in, sz);
			s->next_out += sz;
			s->next_in += sz;
			s->avail_out -= sz;
			s->avail_in -= sz;
			s->raw -= sz;
			continue;
		}

		if (s->avail_in < 8)
			return CAPN_NEED_MORE;

		sz = 0;
		for (i = 0; i < 8; i++) {
			if (s->next_in[i]) {
				sz ++;
				hdr |= 1 << i;
			}
		}

		switch (sz) {
		case 0:
			if (s->avail_out < 2)
				return CAPN_NEED_MORE;

			s->next_out[0] = 0;
			for (sz = 1; sz < min(s->avail_in/8, 256); sz++) {
				if (((uint64_t*) s->next_in)[sz] != 0) {
					break;
				}
			}

			s->next_out[1] = (uint8_t) (sz-1);
			s->next_in += sz*8;
			s->avail_in -= sz*8;
			s->next_out += 2;
			s->avail_out -= 2;
			continue;

		case 8:
			if (s->avail_out < 10)
				return CAPN_NEED_MORE;

			s->next_out[0] = 0xFF;
			memcpy(s->next_out+1, s->next_in, 8);
			s->next_in += 8;
			s->avail_in -= 8;

			s->raw = min(s->avail_in, 256*8);
			if ((p = (uint8_t*) memchr(s->next_in, 0, s->raw)) != NULL) {
				s->raw = (p - s->next_in) & ~7;
			}

			s->next_out[9] = (uint8_t) (s->raw/8);
			s->next_out += 10;
			s->avail_out -= 10;
			continue;

		default:
			if (s->avail_out < 1U + sz)
				return CAPN_NEED_MORE;

			*(s->next_out++) = hdr;
			for (i = 0; i < 8; i++) {
				if (s->next_in[i]) {
					*(s->next_out++) = s->next_in[i];
				}
			}
			s->avail_out -= sz + 1;
			s->next_in += 8;
			s->avail_in -= 8;
			continue;
		}
	}

	return 0;
}

int capn_inflate(struct capn_stream* s) {
	while (s->avail_out) {
		int i;
		size_t sz;
		uint8_t hdr;
		uint8_t *wr;

		if (s->avail_buf && s->avail_out >= s->avail_buf) {
			memcpy(s->next_out, s->inflate_buf, s->avail_buf);
			s->next_out += s->avail_buf;
			s->avail_out -= s->avail_buf;
			s->avail_buf = 0;
			if (!s->avail_out)
				return 0;
		}
		if (s->avail_buf && s->avail_out < s->avail_buf) {
			memcpy(s->next_out, s->inflate_buf, s->avail_out);
			memmove(s->inflate_buf, s->inflate_buf + s->avail_out,
					s->avail_buf - s->avail_out);
			s->avail_buf -= s->avail_out;
			s->avail_out = 0;
			return 0;
		}

		if (s->zeros > 0) {
			sz = min(s->avail_out, s->zeros);
			memset(s->next_out, 0, sz);
			s->next_out += sz;
			s->avail_out -= sz;
			s->zeros -= sz;
			continue;
		}

		if (s->raw > 0) {
			if (s->avail_in == 0)
				return CAPN_NEED_MORE;

			sz = min(min(s->avail_out, s->raw), s->avail_in);
			memcpy(s->next_out, s->next_in, sz);
			s->next_in += sz;
			s->next_out += sz;
			s->avail_in -= sz;
			s->avail_out -= sz;
			s->raw -= sz;
			continue;
		}

		if (s->avail_in == 0)
			return 0;
		else if (s->avail_in < 2)
			return CAPN_NEED_MORE;

		switch (s->next_in[0]) {
		case 0xFF:
			/* 0xFF is followed by 8 bytes raw, followed by
			 * a byte with length in words to read raw */
			if (s->avail_in < 10)
				return CAPN_NEED_MORE;

			memcpy(s->inflate_buf, s->next_in+1, 8);
			s->avail_buf = 8;

			s->raw = s->next_in[9] * 8;
			s->next_in += 10;
			s->avail_in -= 10;
			continue;

		case 0x00:
			/* 0x00 is followed by a single byte indicating
			 * the count of consecutive zero value words
			 * minus 1 */
			s->zeros = (s->next_in[1] + 1) * 8;
			s->next_in += 2;
			s->avail_in -= 2;
			continue;

		default:
			hdr = s->next_in[0];
			sz = 0;
			for (i = 0; i < 8; i++) {
				if (hdr & (1 << i))
					sz++;
			}
			if (s->avail_in < 1U + sz)
				return CAPN_NEED_MORE;

			s->next_in += 1;

			wr = s->inflate_buf;
			for (i = 0; i < 8; i++) {
				if (hdr & (1 << i)) {
					*wr++ = *s->next_in++;
				} else {
					*wr++ = 0;
				}
			}

			s->avail_buf = 8;
			s->avail_in -= 1 + sz;
			continue;
		}
	}

	return 0;
}

