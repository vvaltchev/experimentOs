
#pragma once

#if defined(__i386__)

#  include <arch/generic_x86/utils.h>
#  include <arch/i386/arch_utils.h>
#  include <arch/i386/process_int.h>

#elif defined(__x86_64__)

#  include <arch/generic_x86/utils.h>

// Hack for making the build of unit tests to pass.
#  if defined(KERNEL_TEST)
#     include <arch/i386/arch_utils.h>
#     include <arch/i386/process_int.h>
#  endif

#else

#error Unsupported architecture.

#endif
