#if !defined(C_LOG_H)
/* ========================================================================
   $File: c_log.h $
   $Date: December 06 2025 09:25 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_LOG_H
#include <c_base.h>
#include <c_types.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef enum debug_log_level
{
    SL_LOG_TRACE,
    SL_LOG_INFO,
    SL_LOG_WARNING,
    SL_LOG_ERROR,
    SL_LOG_FATAL
}debug_log_level_t;

#define Log(log_level, message, ...) _log(log_level, message, __FILE__, __LINE__, ##__VA_ARGS__)

#define log_trace(message, ...)    Log(SL_LOG_TRACE,   message, ##__VA_ARGS__)
#define log_info(message, ...)     Log(SL_LOG_INFO,    message, ##__VA_ARGS__)
#define log_warning(message, ...)  Log(SL_LOG_WARNING, message, ##__VA_ARGS__)
#define log_error(message, ...)    Log(SL_LOG_ERROR,   message, ##__VA_ARGS__)
#define log_fatal(message, ...)    Log(SL_LOG_FATAL,   message, ##__VA_ARGS__)

inline void
_log(debug_log_level_t log_level, const char *message, char *file, s32 line, ...)
{
    const char *info_strings[] = {"[TRACE]: ", "[INFO]: ", "[WARNING]:", "[NON-FATAL ERROR]: ", "[FATAL ERROR]: "};
    const char *color_schemes[]  =
    {
        "\033[36m",          // LOG_TRACE:   Teal
        "\033[32m",          // LOG_INFO:    Green
        "\033[33m",          // LOG_WARNING: Yellow
        "\033[31m",          // LOG_ERROR:   Red
        "\033[41m\033[30m"   // LOG_FATAL:   Red background, Black text
    };
    bool8 is_error = (log_level > 1);

    char buffer[32000];
    memset(buffer, 0, sizeof(buffer));

    va_list arg_ptr;
    va_start(arg_ptr, line);
    vsnprintf(buffer, sizeof(buffer), message, arg_ptr);
    va_end(arg_ptr);

    char out_buffer[32000];
    memset(out_buffer, 0, sizeof(out_buffer));

    if(is_error)
    {
        sprintf(out_buffer, "%s%s[File: %s, Line %d] %s\033[0m\n", color_schemes[log_level], info_strings[log_level], file, line, buffer);
    }
    else
    {
        sprintf(out_buffer, "%s%s%s\033[0m", color_schemes[log_level], info_strings[log_level], buffer);
    }

    if(is_error) fprintf(stderr, "%s", out_buffer);
    else         fprintf(stdout, "%s", out_buffer);
}


#endif // C_LOG_H

