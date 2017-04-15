
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void __wrap_printk(const char *fmt, ...)
{
	printf("[kernel] ");

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

void __wrap_panic(const char *fmt, ...)
{
	printf("--- KERNEL_PANIC ---\n");

	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	printf("--- END PANIC MESSAGE --- n");
   abort();
}

void __wrap_assert_failed(const char *expr, const char *file, int line)
{
	printf("Kernel assertion '%s' FAILED in %s at line %d\n", expr, file, line);
   abort();
}

void __wrap_not_reached(const char *file, int line)
{
   printf("Kernel NOT_REACHED statement in %s at line %d\n", file, line);
   abort();
}
