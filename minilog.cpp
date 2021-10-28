/**
minilog

MIT License

Copyright (c) 2021 Sergey Kosarevsky

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

#include "minilog.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <mutex>

#if defined(_WIN32) || defined(_WIN64)
#	define OS_WINDOWS	1
#endif

#if defined(__APPLE__)
#	define OS_MACOS 1
#endif

#if defined (__ANDROID__)
#	define OS_ANDROID 1
#endif

#if OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	define NOIME
#	define WIN32_SUP
#	include <windows.h>
#else
#	include <sys/time.h>
#	include <pthread.h>
#endif

#if OS_ANDROID
#	include <android/log.h>
#endif

namespace {
	minilog::LogConfig config = {};
	FILE* logFile = nullptr;
	std::mutex logMutex;
} // namespace

static constexpr uint32_t kMaxProcsNesting = 128;

struct ThreadLogContext
{
	uint64_t threadId = 0;
	const char* threadName = nullptr;
	// callstack
	const char* procs[kMaxProcsNesting];
	uint32_t procsNestingLevel = 0;
	bool hasLogsOnThisLevel[kMaxProcsNesting] = { false };
};

bool minilog::initialize(const minilog::LogConfig& cfg)
{
	if (logFile)
		deinitialize();

	logFile = fopen(cfg.fileName, "w");

	if (!logFile)
		return false;

	setCurrentThreadName(cfg.mainThreadName);

	config = cfg;

	if (config.writeIntro)
	{
		log(minilog::Log, "minilog: initializing ...");
		log(minilog::Log, "minilog: log file: %s", cfg.fileName);
	}

	return true;
}

void minilog::deinitialize()
{
	if (!logFile)
		return;

	if (config.writeOutro)
		log(minilog::Log, "minilog: deinitializing...");

	fflush(logFile);
	fclose(logFile);

	logFile = nullptr;
}

static uint64_t getCurrentThreadHandle()
{
#if OS_WINDOWS
	return GetCurrentThreadId();
#elif OS_MACOS
	return pthread_mach_thread_np(pthread_self());
#else
	return pthread_self();
#endif
}

static ThreadLogContext* getThreadLogContext()
{
	static thread_local ThreadLogContext ctx;

	if (!ctx.threadId)
		ctx.threadId = getCurrentThreadHandle();

	return &ctx;
}

static uint32_t getCurrentMilliseconds()
{
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

static char* writeTimeStamp(char* buffer, const char* bufferEnd)
{
	time_t tempTime;
	time(&tempTime);
	::tm tmTime = *localtime(&tempTime);

	const int n = snprintf(buffer, uint32_t(bufferEnd-buffer), "%02d:%02d:%02d.%03d   ", tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec, getCurrentMilliseconds());

	return buffer + n;
}

static char* writeCurrentProcsNesting(char* buffer, const char* bufferEnd)
{
	ThreadLogContext* ctx = getThreadLogContext();

	for (int i = 0; i != ctx->procsNestingLevel; i++)
	{
		const size_t len = strlen(ctx->procs[i]);
		if (buffer + len >= bufferEnd)
			break;
		strncpy(buffer, ctx->procs[i], len);
		buffer += len;
	}
	return buffer;
}

static void writeMessageToLog(const char* msg, const ThreadLogContext* ctx)
{
#if OS_ANDROID
	if (ctx->threadName)
		__android_log_print(ANDROID_LOG_INFO, "minilog", "(%s):%s", ctx->threadName, msg);
	else
		__android_log_print(ANDROID_LOG_INFO, "minilog", "(%llu):%s", (unsigned long long)ctx->threadId, msg);
#endif

	if (!logFile)
		return;

	if (ctx->threadName)
		fprintf(logFile, "(%s):%s\n", ctx->threadName, msg);
	else
		fprintf(logFile, "(%llu):%s\n", (unsigned long long)ctx->threadId, msg);

	if (config.forceFlush)
		fflush(logFile);
}

void minilog::setCurrentThreadName(const char* name)
{
	ThreadLogContext* ctx = getThreadLogContext();

	ctx->threadName = name;
}

void minilog::log(eLogLevel level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	log(level, format, args);
	va_end(args);
}

void minilog::log(eLogLevel level, const char* format, va_list args)
{
	if (level < config.logLevel)
		return;

	constexpr uint32_t kBufferLength = 8192;

	char buffer[kBufferLength];
	const char* bufferEnd = buffer + kBufferLength - 1;

	char* scratchBuf = writeTimeStamp(buffer, bufferEnd);
	scratchBuf = writeCurrentProcsNesting(scratchBuf, bufferEnd);

	vsnprintf(scratchBuf, uint32_t(bufferEnd - scratchBuf), format, args);

	std::lock_guard<std::mutex> lock(logMutex);

	ThreadLogContext* ctx = getThreadLogContext();

	if (ctx->procsNestingLevel > 0)
		ctx->hasLogsOnThisLevel[ctx->procsNestingLevel] = true;

	writeMessageToLog(buffer, ctx);

	if (level >= config.logLevelPrintToConsole)
	{
		if (config.coloredConsole)
		{
#if OS_WINDOWS
			auto getAttr = [](minilog::eLogLevel level) -> WORD
			{
				switch (level) {
					case Paranoid:   return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
					case Debug:      return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
					case Log:        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
					case Warning:    return FOREGROUND_RED | FOREGROUND_INTENSITY;
					case FatalError: return FOREGROUND_RED | FOREGROUND_INTENSITY;
				}
				return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			};
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), getAttr(level));
#else
			if (level >= Warning)
				printf("\033[1;31m");
#endif // OS_WINDOWS
		}

		if (ctx->threadName)
			printf("(%s):%s\n", ctx->threadName, buffer);
		else
			printf("(%llu):%s\n", (unsigned long long)ctx->threadId, buffer);

		if (config.coloredConsole)
		{
#if OS_WINDOWS
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
			if (level >= Warning)
				printf("\033[0m");
#endif // OS_WINDOWS
		}
	}
}

void minilog::logRaw(eLogLevel level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	logRaw(level, format, args);
	va_end(args);
}

void minilog::logRaw(eLogLevel level, const char* format, va_list args)
{
	constexpr uint32_t kBufferLength = 8192;

	char buffer[kBufferLength];

	vsnprintf(buffer, kBufferLength - 1, format, args);

	std::lock_guard<std::mutex> lock(logMutex);

	ThreadLogContext* ctx = getThreadLogContext();

	if (ctx->procsNestingLevel > 0)
		ctx->hasLogsOnThisLevel[ctx->procsNestingLevel] = true;

	writeMessageToLog(buffer, ctx);
}

bool minilog::pushProc(const char* name)
{
	ThreadLogContext* ctx = getThreadLogContext();

	ctx->procs[ ctx->procsNestingLevel ] = name;
	ctx->hasLogsOnThisLevel[ ctx->procsNestingLevel ] = false;
	ctx->procsNestingLevel++;

	assert(ctx->procsNestingLevel < kMaxProcsNesting);

	return ctx->procsNestingLevel < kMaxProcsNesting;
}

void minilog::popProc()
{
	ThreadLogContext* ctx = getThreadLogContext();

	assert(ctx->procsNestingLevel > 0);

	if (ctx->hasLogsOnThisLevel[ctx->procsNestingLevel])
		log(Debug, "<-");

	ctx->procsNestingLevel--;
}
