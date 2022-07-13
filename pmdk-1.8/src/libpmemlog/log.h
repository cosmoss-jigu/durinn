/*
 * Copyright 2014-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * log.h -- internal definitions for libpmem log module
 */

#ifndef LOG_H
#define LOG_H 1

#include <stdint.h>
#include <stddef.h>
#include <endian.h>

#include "ctl.h"
#include "util.h"
#include "os_thread.h"
#include "pool_hdr.h"
#include "page_size.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "alloc.h"
#include "fault_injection.h"

#define PMEMLOG_LOG_PREFIX "libpmemlog"
#define PMEMLOG_LOG_LEVEL_VAR "PMEMLOG_LOG_LEVEL"
#define PMEMLOG_LOG_FILE_VAR "PMEMLOG_LOG_FILE"

/* attributes of the log memory pool format for the pool header */
#define LOG_HDR_SIG "PMEMLOG"	/* must be 8 bytes including '\0' */
#define LOG_FORMAT_MAJOR 1

#define LOG_FORMAT_FEAT_DEFAULT \
	{POOL_FEAT_COMPAT_DEFAULT, POOL_FEAT_INCOMPAT_DEFAULT, 0x0000}

#define LOG_FORMAT_FEAT_CHECK \
	{POOL_FEAT_COMPAT_VALID, POOL_FEAT_INCOMPAT_VALID, 0x0000}

static const features_t log_format_feat_default = LOG_FORMAT_FEAT_DEFAULT;

struct pmemlog {
	struct pool_hdr hdr;	/* memory pool header */

	/* root info for on-media format... */
	uint64_t start_offset;	/* start offset of the usable log space */
	uint64_t end_offset;	/* maximum offset of the usable log space */
	uint64_t write_offset;	/* current write point for the log */

	/* some run-time state, allocated out of memory pool... */
	void *addr;		/* mapped region */
	size_t size;		/* size of mapped region */
	int is_pmem;		/* true if pool is PMEM */
	int rdonly;		/* true if pool is opened read-only */
	os_rwlock_t *rwlockp;	/* pointer to RW lock */
	int is_dev_dax;		/* true if mapped on device dax */
	struct ctl *ctl;	/* top level node of the ctl tree structure */

	struct pool_set *set;	/* pool set info */
};

/* data area starts at this alignment after the struct pmemlog above */
#define LOG_FORMAT_DATA_ALIGN ((uintptr_t)PMEM_PAGESIZE)

/*
 * log_convert2h -- convert pmemlog structure to host byte order
 */
static inline void
log_convert2h(struct pmemlog *plp)
{
	plp->start_offset = le64toh(plp->start_offset);
	plp->end_offset = le64toh(plp->end_offset);
	plp->write_offset = le64toh(plp->write_offset);
}

/*
 * log_convert2le -- convert pmemlog structure to LE byte order
 */
static inline void
log_convert2le(struct pmemlog *plp)
{
	plp->start_offset = htole64(plp->start_offset);
	plp->end_offset = htole64(plp->end_offset);
	plp->write_offset = htole64(plp->write_offset);
}

#if FAULT_INJECTION
void
pmemlog_inject_fault_at(enum pmem_allocation_type type, int nth,
							const char *at);

int
pmemlog_fault_injection_enabled(void);
#else
static inline void
pmemlog_inject_fault_at(enum pmem_allocation_type type, int nth,
							const char *at)
{
	abort();
}

static inline int
pmemlog_fault_injection_enabled(void)
{
	return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
