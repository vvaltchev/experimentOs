
#include <string_util.h>
#include <term.h>

#define MAGIC_ITOA_STRING \
   "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"


// Two kind-of ugly macros to avoid code duplication.

#define instantiate_generic_itoa(function_name, integer_type)   \
   void function_name(integer_type value, char *destBuf)        \
   {                                                            \
      const integer_type base = 10;                             \
      char *ptr = destBuf;                                      \
                                                                \
      if (value < 0) {                                          \
         *ptr++ = '-';                                          \
      }                                                         \
                                                                \
      char *low = ptr;                                          \
                                                                \
      do {                                                      \
         /* Mod(x, b) < 0 if x < 0: no need for abs(). */       \
         *ptr++ = MAGIC_ITOA_STRING[35 + value % base];         \
         value /= base;                                         \
      } while ( value );                                        \
                                                                \
      *ptr-- = '\0';                                            \
                                                                \
      while (low < ptr) {                                       \
         char tmp = *low;                                       \
         *low++ = *ptr;                                         \
         *ptr-- = tmp;                                          \
      }                                                         \
   }

#define instantiate_generic_uitoa(function_name, integer_type)     \
   void function_name(integer_type value, char *destBuf, u32 base) \
   {                                                               \
      char *ptr = destBuf;                                         \
      char *low = ptr;                                             \
                                                                   \
      do {                                                         \
         /* Mod(x, b) < 0 if x < 0: no need for abs(). */          \
         *ptr++ = MAGIC_ITOA_STRING[35 + value % base];            \
         value /= base;                                            \
      } while (value);                                             \
                                                                   \
      *ptr-- = '\0';                                               \
      while (low < ptr) {                                          \
         char tmp = *low;                                          \
         *low++ = *ptr;                                            \
         *ptr-- = tmp;                                             \
      }                                                            \
   }                                                               \


instantiate_generic_itoa(itoa32, s32)
instantiate_generic_itoa(itoa64, s64)
instantiate_generic_uitoa(uitoa32, u32)
instantiate_generic_uitoa(uitoa64, u64)

void vprintk(const char *fmt, va_list args)
{
   const char *ptr = fmt;
   char buf[64];

   while (*ptr) {

      if (*ptr != '%') {
         term_write_char(*ptr++);
         continue;
      }

      // *ptr is '%'

      ++ptr;

      if (*ptr == '%')
         continue;

      switch (*ptr) {

      case 'l':
         ++ptr;

         if (*ptr && *ptr == 'l') {
            ++ptr;
            if (*ptr) {
               if (*ptr == 'u') {
                  uitoa(va_arg(args, u64), buf, 10);
                  term_write_string(buf);
               } else if (*ptr == 'i' || *ptr == 'd') {
                  itoa(va_arg(args, s64), buf);
                  term_write_string(buf);
               }
            }
         }
         break;

      case 'd':
      case 'i':
         itoa(va_arg(args, s32), buf);
         term_write_string(buf);
         break;

      case 'u':
         uitoa(va_arg(args, u32), buf, 10);
         term_write_string(buf);
         break;

      case 'x':
         uitoa(va_arg(args, u32), buf, 16);
         term_write_string(buf);
         break;

      case 'c':
         term_write_char(va_arg(args, s32));
         break;

      case 's':
         term_write_string(va_arg(args, const char *));
         break;

      case 'p':
         uitoa(va_arg(args, uptr), buf, 16);
         size_t len = strlen(buf);
         size_t fixedLen = 2 * sizeof(void*);

         term_write_string("0x");

         while (fixedLen-- > len) {
            term_write_char('0');
         }

         term_write_string(buf);
         break;

      default:
         term_write_char('%');
         term_write_char(*ptr);
      }

      ++ptr;
   }
}

#include <hal.h>

void printk(const char *fmt, ...)
{
   disable_preemption();
   {
      va_list args;
      va_start(args, fmt);
      vprintk(fmt, args);
   }
   enable_preemption();
}
