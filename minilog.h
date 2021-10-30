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
		bool forceFlush = true;                           // call fflush() after every log() and logRaw()
		bool writeIntro = true;
		bool writeOutro = true;
		bool coloredConsole = true;                       // apply colors to console output (Windows, escape sequences)
		bool htmlLog = false;                             // output everything as HTML instead of plain text
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

	/// set up custom callbacks
	struct LogCallback
	{
		typedef void (*callback_t)(void*, const char*);
		callback_t funcs[minilog::FatalError + 1] = {};
		void* userData = nullptr;
	};
	bool callbackAdd(const LogCallback& cb);
	void callbackRemove(void* userData);

	/// RAII wrapper around callstackPushProc() and callstackPopProc()
	class CallstackScope
	{
		enum { kBufferSize = 256 };
	public:
		explicit CallstackScope(const char* funcName);
		explicit CallstackScope(const char* funcName, const char* format, ...);
		inline ~CallstackScope()
		{
			minilog::callstackPopProc();
		}
	private:
		char buffer_[kBufferSize];
	};
} // namespace minilog
