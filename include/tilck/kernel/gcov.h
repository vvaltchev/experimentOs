/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

void gcov_dump_coverage(void);

int sys_gcov_dump_coverage(void);
int sys_gcov_get_file_info(int fn, char *user_fname_buf, u32 *user_fsize);
