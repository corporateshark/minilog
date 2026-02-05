#include "minilog.h"

#include <thread>

#if defined(__GNUC__)
#define FUNC_NAME __PRETTY_FUNCTION__
#else
#define FUNC_NAME __FUNCTION__
#endif

void testTXT() {
  minilog::initialize("log.txt", {});

  minilog::callstackPushProc("testTXT()->");
  minilog::log(minilog::Log, "Hello world!");
  minilog::log(minilog::Warning, "Warning!!!");
  minilog::callstackPopProc();

  minilog::deinitialize();
}

void testHTML() {
  minilog::initialize("log.html", {.htmlLog = true});

  minilog::callstackPushProc("testHTML()->");
  minilog::log(minilog::Log, "Hello world!");
  minilog::log(minilog::Warning, "Warning!!!");
  minilog::callstackPopProc();

  minilog::deinitialize();
}

void testThread() {
  minilog::initialize("log_thread.html", {.htmlLog = true});

  std::thread t([]() {
    minilog::threadNameSet("OtherThread");
    minilog::callstackPushProc("std::thread->");
    minilog::log(minilog::Log, "Hello from another thread!");
    minilog::log(minilog::Warning, "Warning from another thread!!!");
    minilog::callstackPopProc();
  });

  minilog::callstackPushProc("testThread()->");
  minilog::log(minilog::Log, "Hello world!");
  minilog::log(minilog::Warning, "Warning!!!");
  minilog::callstackPopProc();

  t.join();

  minilog::deinitialize();
}

void testCallstack() {
  minilog::initialize("log_callstack.txt", {});

  {
    minilog::CallstackScope scope(FUNC_NAME);

    minilog::log(minilog::Log, "Hello world!");
    minilog::log(minilog::Warning, "Warning!!!");
  }

  minilog::deinitialize();
}

void testCallstackMacros() {
  minilog::initialize("log_callstack_macros.txt", {});

  const unsigned int i = 32167;

  {
    minilog::CallstackScope scope(FUNC_NAME);

    LLOGL("Hello world!");
    LLOGW("Warning!!! i = %u", i);
  }

  minilog::deinitialize();
}

void testCallbacks() {
  minilog::initialize("log_callbacks.txt", {});

  // intercept formatted messages
  minilog::LogCallback cb = {.userData = nullptr};
  cb.funcs[minilog::Warning] = [](void* userData, const char* msg) { printf(">>> CALLBACK Warning: %s\n", msg); };
  minilog::callbackAdd(cb);

  const unsigned int i = 32167;

  {
    minilog::CallstackScope scope(FUNC_NAME);

    LLOGL("Hello world!");
    LLOGW("Warning!!! i = %u", i);
  }

  minilog::deinitialize();
}

char* writeCustomTimeStamp(char* buffer, const char* bufferEnd) {
  time_t tempTime;
  time(&tempTime);
  ::tm tmTime = *localtime(&tempTime);

  const char* monthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  const int n = snprintf(buffer,
                         uint32_t(bufferEnd - buffer),
                         "%04d.%s.%02d-%02d:%02d:%02d.%03d   ",
                         tmTime.tm_year + 1900,
                         monthName[tmTime.tm_mon],
                         tmTime.tm_mday,
                         tmTime.tm_hour,
                         tmTime.tm_min,
                         tmTime.tm_sec,
                         minilog::getCurrentMilliseconds());

  return buffer + n;
}

void testCustomTimestamp() {
  minilog::initialize("log_timestamps.txt", {.writeTimeStamp = &writeCustomTimeStamp});

  minilog::callstackPushProc("testCustomTimeStamp()->");
  minilog::log(minilog::Log, "New Time Stamp!");
  minilog::log(minilog::Warning, "Another Time Stamp!!!");
  minilog::callstackPopProc();

  minilog::deinitialize();
}

int main() {
  testTXT();
  testHTML();
  testThread();
  testCallstack();
  testCallstackMacros();
  testCallbacks();
  testCustomTimestamp();

  return 0;
}
