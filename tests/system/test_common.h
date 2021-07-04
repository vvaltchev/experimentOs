/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include "devshell.h"

void create_test_file(const char *path, int n);
int remove_test_file(const char *path, int n);
void remove_test_file_expecting_success(const char *path, int n);
bool running_on_tilck(void);
void not_on_tilck_message(void);

int test_sig(void (*child_func)(void *),
             void *arg,
             int ex_sig,
             int ex_code,
             int signal_to_send);
