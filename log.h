/**
 * Simple logging module
 * (c) Sergey Shcherbakov <shchers@gmail.com>
 */

#include <stdio.h>

#ifndef _LOG_H_
#define _LOG_H_

#ifndef TAG
#define TAG "empty"
#warning "TAG" not defined
#endif

#ifdef LOG_ENABLE_COLORS
#ifndef __linux__
#warning Be careful, colored log can be used mostly in Linux console
#endif
#define LOG_COLOR_RED            "\033[31m"
#define LOG_COLOR_YELLOW         "\033[33m"
#define LOG_COLOR_GREEN          "\033[32m"
#define LOG_COLOR_LIGHT_GRAY     "\033[37m"
#define LOG_COLOR_DARK_GRAY      "\033[90m"
#define LOG_COLOR_END            "\033[0m"
#else
#define LOG_COLOR_RED            ""
#define LOG_COLOR_YELLOW         ""
#define LOG_COLOR_GREEN          ""
#define LOG_COLOR_LIGHT_GRAY     ""
#define LOG_COLOR_DARK_GRAY      ""
#define LOG_COLOR_END            ""
#endif

#define LOG(_TYPE, _TAG, ...) do {\
        fprintf(stderr, _TYPE"/"_TAG": "__VA_ARGS__); \
        fprintf(stderr, LOG_COLOR_END"\r\n"); \
    } while(0)
#define LOGV(...) LOG(LOG_COLOR_DARK_GRAY"V", TAG, ##__VA_ARGS__)
#define LOGD(...) LOG(LOG_COLOR_LIGHT_GRAY"D", TAG, ##__VA_ARGS__)
#define LOGI(...) LOG(LOG_COLOR_GREEN"I", TAG, ##__VA_ARGS__)
#define LOGW(...) LOG(LOG_COLOR_YELLOW"W", TAG, ##__VA_ARGS__)
#define LOGE(...) LOG(LOG_COLOR_RED"E", TAG, ##__VA_ARGS__)

#endif // _LOG_H_
