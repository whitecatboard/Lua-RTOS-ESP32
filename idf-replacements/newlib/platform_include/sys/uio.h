/*
 * SPDX-FileCopyrightText: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// WHITECAT BEGIN
#if 0
struct iovec;
#endif

struct iovec {
	void *iov_base;
	size_t iov_len;
};
// WHITECAT END

int writev(int s, const struct iovec *iov, int iovcnt);

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

#ifdef __cplusplus
}
#endif
