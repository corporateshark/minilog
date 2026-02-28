/**
minilog

MIT License

Copyright (c) 2021-2026 Sergey Kosarevsky

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**/

#if (defined(_WIN32) || defined(_WIN64)) && !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "minilog.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <mutex>

#if !defined(MINILOG_ENABLE_VA_LIST)
// forward declaractions
namespace minilog {
void log(eLogLevel level, const char* format, va_list args);
void logRaw(eLogLevel level, const char* format, va_list args);
} // namespace minilog
#endif // MINILOG_ENABLE_VA_LIST

// clang-format off
#if defined(_WIN32) || defined(_WIN64)
#  define OS_WINDOWS 1
#endif

#if defined(__APPLE__)
#  define OS_MACOS 1
#endif

#if defined(__ANDROID__)
#  define OS_ANDROID 1
#endif

#if OS_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  define NOMINMAX
#  define NOIME
#  define WIN32_SUP
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <pthread.h>
#endif

#if OS_ANDROID
#  include <android/log.h>
#endif

#if OS_MACOS
#  include <os/log.h>
#endif
// clang-format on

static constexpr uint32_t kMaxProcsNesting = 128;
static constexpr uint32_t kMaxCallbacks = 128;

namespace {
minilog::LogConfig config = {};
FILE* logFile = nullptr;
std::mutex logMutex;
minilog::LogCallback callbacks[kMaxCallbacks];
uint32_t callbacksNum = 0;
} // namespace

struct ThreadLogContext {
  uint64_t threadId = 0;
  const char* threadName = nullptr;
  // callstack
  const char* procs[kMaxProcsNesting];
  uint32_t procsNestingLevel = 0;
  bool hasLogsOnThisLevel[kMaxProcsNesting] = {false};
};

#if OS_MACOS
static os_log_type_t logLevelToOsLogType(minilog::eLogLevel level) {
  switch (level) {
  case minilog::eLogLevel::Paranoid:
    return OS_LOG_TYPE_INFO;
  case minilog::eLogLevel::Debug:
    return OS_LOG_TYPE_DEBUG;
  case minilog::eLogLevel::Log:
    return OS_LOG_TYPE_DEFAULT;
  case minilog::eLogLevel::Warning:
    return OS_LOG_TYPE_ERROR;
  case minilog::eLogLevel::FatalError:
    return OS_LOG_TYPE_FAULT;
  }
  return OS_LOG_TYPE_DEFAULT;
}
#endif

static void invokeCallbacks(minilog::eLogLevel level, const char* msg) {
  for (uint32_t i = 0; i != callbacksNum; i++) {
    if (callbacks[i].funcs[level])
      callbacks[i].funcs[level](callbacks[i].userData, msg);
  }
}

static void writeHTMLIntro(const char* pageTitle, const char* customHeader) {
  if (!logFile)
    return;

  const char* header =
      customHeader
          ? customHeader
          : "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" /><title>%s</title>"
            "<style type=\"text/css\">"
            "body{background-color: #061920;padding: 0px;}"
            "h1{font-size: 18pt; font-family: Arial; color: #C9D6D6;margin: 20px;}"
            "h2{font-size: 10pt; font-family: Arial; color: #C9D6D6;margin: 0px;padding-top: 10px;}"

            "#l1{background-color: #39464C;font-size: 10pt; font-family: Arial; color: white;padding-left: 5px;margin-bottom: 1px;}"
            "#l2{background-color: #39464C;font-size: 10pt; font-family: Arial; color: #AAAAAA;padding-left: 5px;margin-bottom: 1px;}"
            "#p1{background-color: #A68600;font-size: 11pt;font-weight: bold;font-family: Arial; color: white;padding-left: "
            "15px;margin-bottom: 1px;}"
            "#p2{background-color: #A68600;font-size: 11pt;font-weight: bold;font-family: Arial; color: #AAAAAA;padding-left: "
            "15px;margin-bottom: 1px;}"
            "#w1{background-color: maroon;font-size: 11pt;font-weight: bold;font-family: Arial; color: white;padding-left: "
            "15px;margin-bottom: 1px;}"
            "#w2{background-color: maroon;font-size: 11pt;font-weight: bold;font-family: Arial; color: #AAAAAA;padding-left: "
            "15px;margin-bottom: 1px;}"

            "</style></head>\n";

  fprintf(logFile, header, pageTitle);
  fprintf(logFile, "<body><h1>%s</h1>\n", pageTitle);
}

static void writeHTMLOutro(const char* customFooter) {
  if (!logFile)
    return;

  const char* footer = customFooter ? customFooter : "</body></html>\n";

  fprintf(logFile, "%s", footer);
}

bool minilog::initialize(const char* fileName, const minilog::LogConfig& cfg) {
  if (logFile)
    deinitialize();

  if (fileName) {
    logFile = fopen(fileName, "w");

    if (!logFile)
      return false;
  }

  minilog::threadNameSet(cfg.mainThreadName);

  config = cfg;

  if (cfg.htmlLog)
    writeHTMLIntro(cfg.htmlPageTitle, cfg.htmlPageHeader);

  if (cfg.writeIntro) {
    log(minilog::Log, "minilog: initializing ...");
    log(minilog::Log, "minilog: log file: %s", fileName);
  }

  return true;
}

void minilog::deinitialize() {
  if (!logFile)
    return;

  if (config.writeOutro)
    log(minilog::Log, "minilog: deinitializing...");

  if (config.htmlLog)
    writeHTMLOutro(config.htmlPageFooter);

  fflush(logFile);
  fclose(logFile);

  logFile = nullptr;
}

static uint64_t getCurrentThreadHandle() {
#if OS_WINDOWS
  return GetCurrentThreadId();
#elif OS_MACOS
  return pthread_mach_thread_np(pthread_self());
#else
  return pthread_self();
#endif
}

static ThreadLogContext* getThreadLogContext() {
  static thread_local ThreadLogContext ctx;

  if (!ctx.threadId)
    ctx.threadId = getCurrentThreadHandle();

  return &ctx;
}

unsigned int minilog::getCurrentMilliseconds() {
#if OS_WINDOWS
  SYSTEMTIME st;
  GetSystemTime(&st);
  return st.wMilliseconds;
#else
  struct timeval timeVal;
  gettimeofday(&timeVal, nullptr);
  return timeVal.tv_usec / 1000;
#endif
}

static char* writeTimeStamp(char* buffer, const char* bufferEnd) {
  time_t tempTime;
  time(&tempTime);
  ::tm tmTime;
#if OS_WINDOWS
  localtime_s(&tmTime, &tempTime);
#else
  localtime_r(&tempTime, &tmTime);
#endif

  const int n = snprintf(buffer,
                         uint32_t(bufferEnd - buffer),
                         "%02d:%02d:%02d.%03d   ",
                         tmTime.tm_hour,
                         tmTime.tm_min,
                         tmTime.tm_sec,
                         minilog::getCurrentMilliseconds());

  return buffer + n;
}

static char* writeCurrentProcsNesting(char* buffer, const char* bufferEnd) {
  ThreadLogContext* ctx = getThreadLogContext();

  for (int i = 0; i != ctx->procsNestingLevel; i++) {
    const size_t len = strlen(ctx->procs[i]);
    if (buffer + len >= bufferEnd)
      break;
    strncpy(buffer, ctx->procs[i], len);
    buffer += len;
  }
  return buffer;
}

static const char* kHTMLPrefix[] = {
    "<div id=\"p1\">", // Paranoid
    "<div id=\"p2\">", // Paranoid
    "<div id=\"l1\">", // Debug
    "<div id=\"l2\">", // Debug
    "<div id=\"l1\">", // Log
    "<div id=\"l2\">", // Log
    "<div id=\"w1\">", // Warning
    "<div id=\"w2\">", // Warning
    "<div id=\"w1\">", // FatalError
    "<div id=\"w2\">" // FatalError
};

static void writeMessageToLog(minilog::eLogLevel level, const char* msg, const ThreadLogContext* ctx) {
#if OS_ANDROID
  if (ctx->threadName)
    __android_log_print(ANDROID_LOG_INFO, "minilog", "(%s):%s", ctx->threadName, msg);
  else
    __android_log_print(ANDROID_LOG_INFO, "minilog", "(%llu):%s", (unsigned long long)ctx->threadId, msg);
#endif

  if (!logFile)
    return;

  if (config.threadNames)
    if (ctx->threadName)
      if (config.htmlLog) {
        const int threadID = strcmp(ctx->threadName, config.mainThreadName) ? 1 : 0;
        fprintf(logFile, "%s(%s):%s</div>\n", kHTMLPrefix[2 * level + threadID], ctx->threadName, msg);
      } else
        fprintf(logFile, "(%s):%s\n", ctx->threadName, msg);
    else if (config.htmlLog)
      fprintf(logFile, "%s(%llu):%s</div>\n", kHTMLPrefix[2 * level], (unsigned long long)ctx->threadId, msg);
    else
      fprintf(logFile, "(%llu):%s\n", (unsigned long long)ctx->threadId, msg);
  else {
    if (config.htmlLog)
      fprintf(logFile, "%s%s</div>\n", kHTMLPrefix[2 * level], msg);
    else
      fprintf(logFile, "%s\n", msg);
  }

  if (config.forceFlush)
    fflush(logFile);
}

void minilog::threadNameSet(const char* name) {
  ThreadLogContext* ctx = getThreadLogContext();

  ctx->threadName = name;
}

const char* minilog::threadNameGet() {
  ThreadLogContext* ctx = getThreadLogContext();

  return ctx->threadName ? ctx->threadName : "";
}

static void printMessageToConsole(minilog::eLogLevel level, const char* msg, const ThreadLogContext* ctx) {
  using namespace minilog;

  if (level >= config.logLevelPrintToConsole) {
    if (config.coloredConsole) {
#if OS_WINDOWS
      auto getAttr = [](minilog::eLogLevel level) -> WORD {
        switch (level) {
        case Paranoid:
          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Debug:
          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Log:
          return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        case Warning:
          return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case FatalError:
          return FOREGROUND_RED | FOREGROUND_INTENSITY;
        }
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
      };
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), getAttr(level));
#else
      // ANSI colors for macOS and Linux terminals
      switch (level) {
      case Paranoid:
        printf("\033[0;90m");
        break;
      case Debug:
        printf("\033[0m");
        break;
      case Log:
        printf("\033[1m");
        break;
      case Warning:
        printf("\033[1;33m");
        break;
      case FatalError:
        printf("\033[1;31m");
        break;
      }
#endif // OS_WINDOWS
    }

    // clang-format off
#if defined(MINILOG_RAW_OUTPUT)
#	define FORMATSTR_THREAD_NAME "(%s):%s"
#	define FORMATSTR_THREAD_ID   "(%llu):%s"
#	define FORMATSTR_NO_THREAD   "%s"
#else
#	define FORMATSTR_THREAD_NAME "(%s):%s\n"
#	define FORMATSTR_THREAD_ID   "(%llu):%s\n"
#	define FORMATSTR_NO_THREAD   "%s\n"
#endif
    // clang-format on

#if OS_MACOS
    if (config.coloredConsole) {
      if (config.threadNames) {
        if (ctx->threadName) {
          os_log_with_type(OS_LOG_DEFAULT, logLevelToOsLogType(level), "(%{public}s):%{public}s", ctx->threadName, msg);
        } else {
          os_log_with_type(OS_LOG_DEFAULT, logLevelToOsLogType(level), "(%{public}llu):%{public}s", (unsigned long long)ctx->threadId, msg);
        }
      } else {
        os_log_with_type(OS_LOG_DEFAULT, logLevelToOsLogType(level), "%{public}s", msg);
      }
    }
#endif // OS_MACOS

    if (config.threadNames) {
      if (ctx->threadName) {
        printf(FORMATSTR_THREAD_NAME, ctx->threadName, msg);
      } else {
        printf(FORMATSTR_THREAD_ID, (unsigned long long)ctx->threadId, msg);
      }
    } else {
      printf(FORMATSTR_NO_THREAD, msg);
    }

    if (config.coloredConsole) {
#if OS_WINDOWS
      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
      printf("\033[0m");
#endif // OS_WINDOWS
    }
  }
}

void minilog::log(eLogLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  log(level, format, args);
  va_end(args);
}

void minilog::log(eLogLevel level, const char* format, va_list args) {
  if (level < config.logLevel)
    return;

  constexpr uint32_t kBufferLength = 8192;

  char buffer[kBufferLength];
  const char* bufferEnd = buffer + kBufferLength - 1;

  char* scratchBuf = config.writeTimeStamp ? config.writeTimeStamp(buffer, bufferEnd) : writeTimeStamp(buffer, bufferEnd);
  scratchBuf = writeCurrentProcsNesting(scratchBuf, bufferEnd);
  const char* msg = scratchBuf; // store where the actual message starts

  vsnprintf(scratchBuf, uint32_t(bufferEnd - scratchBuf), format, args);

  std::lock_guard<std::mutex> lock(logMutex);

  ThreadLogContext* ctx = getThreadLogContext();

  if (ctx->procsNestingLevel > 0)
    ctx->hasLogsOnThisLevel[ctx->procsNestingLevel] = true;

  writeMessageToLog(level, buffer, ctx);
  printMessageToConsole(level, buffer, ctx);

  invokeCallbacks(level, msg);
}

void minilog::logRaw(eLogLevel level, const char* format, ...) {
  va_list args;
  va_start(args, format);
  logRaw(level, format, args);
  va_end(args);
}

void minilog::logRaw(eLogLevel level, const char* format, va_list args) {
  constexpr uint32_t kBufferLength = 8192;

  char buffer[kBufferLength];

  vsnprintf(buffer, kBufferLength - 1, format, args);

  std::lock_guard<std::mutex> lock(logMutex);

  ThreadLogContext* ctx = getThreadLogContext();

  if (ctx->procsNestingLevel > 0)
    ctx->hasLogsOnThisLevel[ctx->procsNestingLevel] = true;

  writeMessageToLog(level, buffer, ctx);

#if defined(MINILOG_RAW_OUTPUT)
  printMessageToConsole(level, buffer, ctx);
#endif // MINILOG_RAW_OUTPUT

  invokeCallbacks(level, buffer);
}

bool minilog::callstackPushProc(const char* name) {
  ThreadLogContext* ctx = getThreadLogContext();

  ctx->procs[ctx->procsNestingLevel] = name;
  ctx->hasLogsOnThisLevel[ctx->procsNestingLevel] = false;
  ctx->procsNestingLevel++;

  assert(ctx->procsNestingLevel < kMaxProcsNesting);

  return ctx->procsNestingLevel < kMaxProcsNesting;
}

void minilog::callstackPopProc() {
  ThreadLogContext* ctx = getThreadLogContext();

  assert(ctx->procsNestingLevel > 0);

  if (ctx->hasLogsOnThisLevel[ctx->procsNestingLevel])
    log(Debug, "<-");

  ctx->procsNestingLevel--;
}

unsigned int minilog::callstackGetNumProcs() {
  ThreadLogContext* ctx = getThreadLogContext();

  return ctx->procsNestingLevel;
}

const char* minilog::callstackGetProc(unsigned int i) {
  ThreadLogContext* ctx = getThreadLogContext();

  return ctx->procs[i];
}

bool minilog::callbackAdd(const LogCallback& cb) {
  if (callbacksNum >= kMaxCallbacks)
    return false;

  callbacks[callbacksNum++] = cb;

  return true;
}

void minilog::callbackRemove(void* userData) {
  for (uint32_t i = 0; i != callbacksNum; i++) {
    if (callbacks[i].userData == userData) {
      callbacks[i] = callbacks[callbacksNum - 1];
      callbacksNum--;
      return;
    }
  }
}

minilog::CallstackScope::CallstackScope(const char* funcName, const char* format, ...) {
  char argsBuf[kBufferSize];

  va_list p;
  va_start(p, format);
  vsnprintf(argsBuf, kBufferSize - 1, format, p);
  va_end(p);
  snprintf(buffer_, kBufferSize - 1, "%s(%s)->", funcName, argsBuf);

  minilog::callstackPushProc(buffer_);
}

minilog::CallstackScope::CallstackScope(const char* funcName) {
#if defined(__GNUC__) || defined(__clang__)
  snprintf(buffer_, kBufferSize - 1, "%s->", funcName);
#else
  snprintf(buffer_, kBufferSize - 1, "%s()->", funcName);
#endif
  minilog::callstackPushProc(buffer_);
}
