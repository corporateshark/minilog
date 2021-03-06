#pragma once

/**
	minilog v1.0.2

	MIT License
	Copyright (c) 2021-2022 Sergey Kosarevsky
**/

#if defined(MINILOG_ENABLE_VA_LIST)
#	include <stdarg.h>
#endif // MINILOG_ENABLE_VA_LIST

namespace minilog
{
	enum eLogLevel {
		Paranoid    = 0,
		Debug       = 1,
		Log         = 2,
		Warning     = 3,
		FatalError  = 4
	};
	struct LogConfig {
		eLogLevel logLevel = minilog::Debug;              // everything >= this level goes to the log file
		eLogLevel logLevelPrintToConsole = minilog::Log;  // everything >= this level is printed to the console (cannot be lower than logLevel)
		bool forceFlush = true;                           // call fflush() after every log() and logRaw()
		bool writeIntro = true;
		bool writeOutro = true;
		bool coloredConsole = true;                       // apply colors to console output (Windows, escape sequences)
		bool htmlLog = false;                             // output everything as HTML instead of plain text
		const char* htmlPageTitle = "Minilog";            // just the title of the resulting HTML page
		const char* htmlPageHeader = nullptr;             // override default HTML header
		const char* htmlPageFooter = nullptr;             // override default HTML footer
		const char* mainThreadName = "MainThread";        // just the name of the thread which calls minilog::initialize()
	};

	bool initialize(const char* fileName, const LogConfig& cfg); // non-thread-safe
	void deinitialize();                                         // non-thread-safe

	void log(eLogLevel level, const char* format, ...);             // thread-safe
	void logRaw(eLogLevel level, const char* format, ...);          // thread-safe
#if defined(MINILOG_ENABLE_VA_LIST)
	void log(eLogLevel level, const char* format, va_list args);    // thread-safe
	void logRaw(eLogLevel level, const char* format, va_list args); // thread-safe
#endif // MINILOG_ENABLE_VA_LIST

	/// threads management
	void threadNameSet(const char* name); // thread-safe
	const char* threadNameGet();          // thread-safe

	/// callstack management
	bool callstackPushProc(const char* name);     // thread-safe
	void callstackPopProc();                      // thread-safe
	unsigned int callstackGetNumProcs();          // thread-safe
	const char* callstackGetProc(unsigned int i); // thread-safe

	/// set up custom callbacks
	struct LogCallback {
		typedef void (*callback_t)(void*, const char*);
		callback_t funcs[minilog::FatalError + 1] = {};
		void* userData = nullptr;
	};
	bool callbackAdd(const LogCallback& cb); // non-thread-safe
	void callbackRemove(void* userData);     // non-thread-safe

	/// RAII wrapper around callstackPushProc() and callstackPopProc()
	class CallstackScope {
		enum { kBufferSize = 256 };
	public:
		explicit CallstackScope(const char* funcName);
		explicit CallstackScope(const char* funcName, const char* format, ...);
		inline ~CallstackScope() { minilog::callstackPopProc(); }
	private:
		char buffer_[kBufferSize];
	};
} // namespace minilog

#if !defined(MINILOG_DISABLE_HELPER_MACROS)

#if defined(__GNUC__) && !defined(EMSCRIPTEN) && !defined(__clang__)
#	define LLOGP(...) minilog::log(minilog::Paranoid, ##__VA_ARGS__)
#	define LLOGD(...) minilog::log(minilog::Debug, ##__VA_ARGS__)
#	define LLOGL(...) minilog::log(minilog::Log, ##__VA_ARGS__)
#	define LLOGW(...) minilog::log(minilog::Warning, ##__VA_ARGS__)
#else
#	define LLOGP(...) minilog::log(minilog::Paranoid, ## __VA_ARGS__)
#	define LLOGD(...) minilog::log(minilog::Debug, ## __VA_ARGS__)
#	define LLOGL(...) minilog::log(minilog::Log, ## __VA_ARGS__)
#	define LLOGW(...) minilog::log(minilog::Warning, ## __VA_ARGS__)
#endif

#endif // MINILOG_DISABLE_HELPER_MACROS
