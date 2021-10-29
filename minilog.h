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
		eLogLevel logLevel = minilog::Debug;
		eLogLevel logLevelPrintToConsole = minilog::Log;
		bool forceFlush = true;
		bool writeIntro = true;
		bool writeOutro = true;
		bool coloredConsole = true;
		bool htmlLog = false;
		const char* htmlPageTitle = "Minilog";
		const char* mainThreadName = "MainThread";
	};

	bool initialize(const char* fileName, const LogConfig& cfg);
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

	struct LogCallback
	{
		typedef void (*callback_t)(void*, const char*);
		callback_t funcs[minilog::FatalError + 1] = {};
		void* userData = nullptr;
	};
	bool callbackAdd(const LogCallback& cb);
	void callbackRemove(void* userData);
} // namespace minilog
