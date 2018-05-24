/* vim: set sw=8 ts=8 sts=8 noet: */
/* capnp_c.h
 *
 * Copyright (C) 2013 James McKaskill
 * Copyright (C) 2014 Steve Dee
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

/*
 * functions / structures in this header are private to the capnproto-c
 * library;  applications should not call or use them.
 */

#ifndef CAPNP_PRIV_H
#define CAPNP_PRIV_H

#include "capnp_c.h"

#if defined(__GNUC__) && __GNUC__ >= 4
# define intern __attribute__((visibility ("internal")))
#else
# define intern /**/
#endif

/* capn_stream encapsulates the needed fields for capn_(deflate|inflate) in a
 * similar manner to z_stream from zlib
 *
 * The user should set next_in, avail_in, next_out, avail_out to the
 * available in/out buffers before calling capn_(deflate|inflate).
 *
 * Other fields should be zero initialized.
 */
struct capn_stream {
	const uint8_t *next_in;
	size_t avail_in;
	uint8_t *next_out;
	size_t avail_out;
	unsigned zeros, raw;

	uint8_t inflate_buf[8];
	size_t avail_buf;
};

#define CAPN_MISALIGNED -1
#define CAPN_NEED_MORE -2

/* capn_deflate deflates a stream to the packed format
 * capn_inflate inflates a stream from the packed format
 *
 * Returns:
 * CAPN_MISALIGNED - if the unpacked data is not 8 byte aligned
 * CAPN_NEED_MORE - more packed data/room is required (out for inflate, in for
 * deflate)
 * 0 - success, all output for inflate, all input for deflate processed
 */
intern int capn_deflate(struct capn_stream*);
intern int capn_inflate(struct capn_stream*);


#endif /* CAPNP_PRIV_H */
