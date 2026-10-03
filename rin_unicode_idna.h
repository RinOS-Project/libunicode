/* SPDX-License-Identifier: MIT */
#ifndef RIN_UNICODE_IDNA_H
#define RIN_UNICODE_IDNA_H

#include "rin_unicode.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RIN_UNICODE_IDNA_MAX_DOMAIN_BYTES 1024u
#define RIN_UNICODE_IDNA_MAX_ASCII_BYTES 253u

enum {
    RIN_UNICODE_IDNA_OK = 0,
    RIN_UNICODE_IDNA_INVALID = -1,
    RIN_UNICODE_IDNA_NO_SPACE = -2
};

/* The Unicode 17.0 UTS #46 non-transitional ToASCII workspace is bounded and
 * caller-owned. Allocate the returned size with at least uint32_t alignment.
 * No pathname, service, or locale state is consulted. */
size_t rin_unicode_idna_workspace_size(void);

/* Convert a UTF-8 domain name to lowercase ASCII A-labels using non-
 * transitional UTS #46, STD3, hyphen, DNS-length, bidi, ContextJ and ContextO
 * checks. Empty labels (including a final root dot) are rejected. Input and
 * output buffers must not overlap workspace or each other. output_length is
 * cleared and output is NUL-terminated when possible on failure. */
int rin_unicode_idna_to_ascii(const char* input, size_t input_length,
                              char* output, size_t output_capacity,
                              size_t* output_length, void* workspace,
                              size_t workspace_size);

#ifdef __cplusplus
}
#endif

#endif /* RIN_UNICODE_IDNA_H */
