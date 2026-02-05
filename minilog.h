#pragma once

/**
  minilog v1.2.0

  MIT License
  Copyright (c) 2021-2026 Sergey Kosarevsky
   https://github.com/corporateshark/minilog
**/

#if defined(MINILOG_ENABLE_VA_LIST)
#include <stdarg.h>
#endif // MINILOG_ENABLE_VA_LIST

namespace minilog {

enum eLogLevel { Paranoid = 0, Debug = 1, Log = 2, Warning = 3, FatalError = 4 };

// A user function to write a time stamp into a buffer `buffer`; it should not write past the pointer `bufferEnd`.
// It returns a pointer to the end of the written data.
using writeTimeStampFn = char* (*)(char* buffer, const char* bufferEnd);

struct LogConfig {
  eLogLevel logLevel = minilog::Debug; // everything >= this level goes to the log file
  eLogLevel logLevelPrintToConsole = minilog::Log; // everything >= this level is printed to the console (cannot be lower than logLevel)
  bool forceFlush = true; // call fflush() after every log() and logRaw()
  bool writeIntro = true;
  bool writeOutro = true;
  bool coloredConsole = true; // apply colors to console output (Windows, macOS, escape sequences)
  bool htmlLog = false; // output everything as HTML instead of plain text
  bool threadNames = true; // prefix log messages with thread names
  const char* htmlPageTitle = "Minilog"; // just the title of the resulting HTML page
  const char* htmlPageHeader = nullptr; // override default HTML header
  const char* htmlPageFooter = nullptr; // override default HTML footer
  const char* mainThreadName = "MainThread"; // just the name of the thread which calls minilog::initialize()
  writeTimeStampFn writeTimeStamp = nullptr; // override default time stamp function
};

bool initialize(const char* fileName, const LogConfig& cfg); // non-thread-safe
void deinitialize(); // non-thread-safe

void log(eLogLevel level, const char* format, ...); // thread-safe
void logRaw(eLogLevel level, const char* format, ...); // thread-safe
#if defined(MINILOG_ENABLE_VA_LIST)
void log(eLogLevel level, const char* format, va_list args); // thread-safe
void logRaw(eLogLevel level, const char* format, va_list args); // thread-safe
#endif // MINILOG_ENABLE_VA_LIST

/// threads management
void threadNameSet(const char* name); // thread-safe
const char* threadNameGet(); // thread-safe

/// callstack management
bool callstackPushProc(const char* name); // thread-safe
void callstackPopProc(); // thread-safe
unsigned int callstackGetNumProcs(); // thread-safe
const char* callstackGetProc(unsigned int i); // thread-safe

/// set up custom callbacks
struct LogCallback {
  typedef void (*callback_t)(void*, const char*);
  callback_t funcs[minilog::FatalError + 1] = {};
  void* userData = nullptr;
};
bool callbackAdd(const LogCallback& cb); // non-thread-safe
void callbackRemove(void* userData); // non-thread-safe

/// RAII wrapper around callstackPushProc() and callstackPopProc()
class CallstackScope {
  enum { kBufferSize = 256 };

 public:
  explicit CallstackScope(const char* funcName);
  explicit CallstackScope(const char* funcName, const char* format, ...);
  inline ~CallstackScope() {
    minilog::callstackPopProc();
  }

 private:
  char buffer_[kBufferSize];
};

unsigned int getCurrentMilliseconds();

} // namespace minilog

// clang-format off
#if defined(MINILOG_RAW_OUTPUT)
#	define MINILOG_LOG_PROC minilog::logRaw
#else
#	define MINILOG_LOG_PROC minilog::log
#endif // MINILOG_RAW_OUTPUT

#if !defined(MINILOG_DISABLE_HELPER_MACROS)

#if defined(__GNUC__) && !defined(EMSCRIPTEN) && !defined(__clang__)
#	define LLOGP(...) MINILOG_LOG_PROC(minilog::Paranoid, ##__VA_ARGS__)
#	define LLOGD(...) MINILOG_LOG_PROC(minilog::Debug, ##__VA_ARGS__)
#	define LLOGL(...) MINILOG_LOG_PROC(minilog::Log, ##__VA_ARGS__)
#	define LLOGW(...) MINILOG_LOG_PROC(minilog::Warning, ##__VA_ARGS__)
#	define LLOGE(...) MINILOG_LOG_PROC(minilog::FatalError, ##__VA_ARGS__)
#else
#	define LLOGP(...) MINILOG_LOG_PROC(minilog::Paranoid, ## __VA_ARGS__)
#	define LLOGD(...) MINILOG_LOG_PROC(minilog::Debug, ## __VA_ARGS__)
#	define LLOGL(...) MINILOG_LOG_PROC(minilog::Log, ## __VA_ARGS__)
#	define LLOGW(...) MINILOG_LOG_PROC(minilog::Warning, ## __VA_ARGS__)
#	define LLOGE(...) MINILOG_LOG_PROC(minilog::FatalError, ## __VA_ARGS__)
#endif

#endif // MINILOG_DISABLE_HELPER_MACROS
// clang-format on
