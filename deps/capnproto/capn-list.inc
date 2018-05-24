/* capn-list.inc
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#define CAT2(A,B) A ## B
#define CAT(A,B) CAT2(A, B)
#define UINT_T CAT(CAT(uint, SZ), _t)
#define LIST_T CAT(capn_list, SZ)
#define FLIP CAT(capn_flip, SZ)

UINT_T CAT(capn_get,SZ) (LIST_T l, int off) {
	char *d;
	capn_ptr p = l.p;
	if (off >= p.len) {
		return 0;
	}

	switch (p.type) {
	case CAPN_LIST:
		if (p.datasz < SZ/8)
			return 0;
		d = p.data + off * (p.datasz + 8*p.ptrs);
		return FLIP(*(UINT_T*)d);

	case CAPN_PTR_LIST:
		d = struct_ptr(p.seg, p.data + 8*off, SZ/8);
		if (d) {
			return FLIP(*(UINT_T*)d);
		} else {
			return 0;
		}

	default:
		return 0;
	}
}

int CAT(capn_getv,SZ) (LIST_T l, int off, UINT_T *to, int sz) {
	int i;
	capn_ptr p;
	capn_resolve(&l.p);
	p = l.p;
	if (off + sz > p.len) {
		sz = p.len - off;
	}

	switch (p.type) {
	case CAPN_LIST:
		if (p.datasz == SZ/8 && !p.ptrs && (SZ == 8 || CAPN_LITTLE)) {
			memcpy(to, p.data + off, sz * (SZ/8));
			return sz;
		} else if (p.datasz < SZ/8) {
			return -1;
		}

		for (i = 0; i < sz; i++) {
			char *d = p.data + (i + off) * (p.datasz + 8*p.ptrs);
			to[i] = FLIP(*(UINT_T*)d);
		}
		return sz;

	case CAPN_PTR_LIST:
		for (i = 0; i < sz; i++) {
			char *d = struct_ptr(p.seg, p.data + 8*(i+off), SZ/8);
			if (d) {
				to[i] = FLIP(*(UINT_T*)d);
			} else {
				return -1;
			}
		}
		return sz;

	default:
		return -1;
	}
}

int CAT(capn_set,SZ) (LIST_T l, int off, UINT_T v) {
	char *d;
	capn_ptr p = l.p;
	if (off >= p.len) {
		return -1;
	}

	switch (p.type) {
	case CAPN_LIST:
		if (p.datasz < SZ/8)
			return -1;
		d = p.data + off * (p.datasz + 8*p.ptrs);
		*(UINT_T*) d = FLIP(v);
		return 0;

	case CAPN_PTR_LIST:
		d = struct_ptr(p.seg, p.data + 8*off, SZ/8);
		if (!d) {
			return -1;
		}
		*(UINT_T*) d = FLIP(v);
		return 0;

	default:
		return -1;
	}
}

int CAT(capn_setv,SZ) (LIST_T l, int off, const UINT_T *from, int sz) {
	int i;
	capn_ptr p = l.p;
	if (off + sz > p.len) {
		sz = p.len - off;
	}

	switch (p.type) {
	case CAPN_LIST:
		if (p.datasz == SZ/8 && !p.ptrs && (SZ == 8 || CAPN_LITTLE)) {
			memcpy(p.data + off, from, sz * (SZ/8));
			return sz;
		} else if (p.datasz < SZ/8) {
			return -1;
		}

		for (i = 0; i < sz; i++) {
			char *d = p.data + (i + off) * (p.datasz + 8*p.ptrs);
			*(UINT_T*) d = FLIP(from[i]);
		}
		return sz;

	case CAPN_PTR_LIST:
		for (i = 0; i < sz; i++) {
			char *d = struct_ptr(p.seg, p.data + 8*(i+off), SZ/8);
			if (d) {
				*(UINT_T*) d = FLIP(from[i]);
			} else {
				return -1;
			}
		}
		return sz;

	default:
		return -1;
	}
}

LIST_T CAT(capn_new_list,SZ) (struct capn_segment *seg, int sz) {
	LIST_T l = {{CAPN_LIST}};
	l.p.seg = seg;
	l.p.len = sz;
	l.p.datasz = SZ/8;
	new_object(&l.p, sz*(SZ/8));
	return l;
}

#undef CAT2
#undef CAT
#undef UINT_T
#undef LIST_T
#undef FLIP

