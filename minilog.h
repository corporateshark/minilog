#pragma once

/**
minilog

MIT License

Copyright (c) 2021 Sergey Kosarevsky
**/

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
	};

	bool initialize(const LogConfig& cfg);
	void deinitialize();
	void log(eLogLevel level, const char* format, ...);
} // namespace minilog
