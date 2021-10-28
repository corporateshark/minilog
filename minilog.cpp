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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#	define OS_WINDOWS	1
#endif

#if OS_WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	define NOMINMAX
#	define NOIME
#	define WIN32_SUP
#	include <windows.h>
#else
#	include <sys/time.h>
#endif

namespace {
	minilog::LogConfig config = {};
	FILE* logFile = nullptr;
} // namespace

bool minilog::initialize(const minilog::LogConfig& cfg)
{
	if (logFile)
		deinitialize();

	logFile = fopen(cfg.fileName, "w");

	if (!logFile)
		return false;

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
		log(minilog::Log, "minilog: deinitializing ...");

	fflush(logFile);
	fclose(logFile);

	logFile = nullptr;
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

static uint32_t writeTimeStamp(char* buffer, uint32_t maxLength)
{
	time_t tempTime;
	time(&tempTime);
	::tm tmTime = *localtime(&tempTime);

	const int n = snprintf(buffer, maxLength, "%02d:%02d:%02d.%03d   ", tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec, getCurrentMilliseconds());

	return n > 0 ? n : 0;
}

static void writeMessageToLog(const char* msg)
{
	if (!logFile)
		return;

	fprintf(logFile, "%s\n", msg);

	if (config.forceFlush)
		fflush(logFile);
}

void minilog::log(eLogLevel level, const char* format, ...)
{
	if (level < config.logLevel)
		return;

	constexpr uint32_t kBufferLength = 8192;

	char buffer[kBufferLength];

	const uint32_t tsLength = writeTimeStamp(buffer, kBufferLength);

	if (tsLength >= kBufferLength)
		return;

	va_list args;
	va_start(args, format);
	vsnprintf(buffer + tsLength, kBufferLength - tsLength, format, args);
	va_end(args);

	writeMessageToLog(buffer);

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

		printf("%s\n", buffer);

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
