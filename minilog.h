#pragma once

/**
minilog

MIT License

Copyright (c) 2021 Sergey Kosarevsky
**/

#include <stdarg.h>

namespace minilog
{
	enum eLogLevel
	{
		Paranoid    = 0,
		Debug       = 1,
		Log         = 2,
		Warning     = 3,
		FatalError  = 4
	};

	struct LogConfig
	{
		const char* fileName = "";
		eLogLevel logLevel = minilog::Debug;
		eLogLevel logLevelPrintToConsole = minilog::Log;
		bool forceFlush = true;
		bool writeIntro = true;
		bool writeOutro = true;
		bool coloredConsole = true;
		const char* mainThreadName = "MainThread";
	};

	bool initialize(const LogConfig& cfg);
	void deinitialize();
	void log(eLogLevel level, const char* format, ...);
	void log(eLogLevel level, const char* format, va_list args);
	void logRaw(eLogLevel level, const char* format, ...);
	void logRaw(eLogLevel level, const char* format, va_list args);
	void threadNameSet(const char* name);
	const char* threadNameGet();
	bool callstackPushProc(const char* name);
	void callstackPopProc();
	unsigned int callstackGetNumProcs();
	const char* callstackGetProc(unsigned int i);
} // namespace minilog
