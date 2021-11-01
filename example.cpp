#include "minilog.h"

#include <thread>

#if defined(__GNUC__)
#	define FUNC_NAME __PRETTY_FUNCTION__
#else
#	define FUNC_NAME __FUNCTION__
#endif

void testTXT()
{
	minilog::initialize("log.txt", {});

	minilog::callstackPushProc("testTXT()->");
	minilog::log(minilog::Log, "Hello world!");
	minilog::log(minilog::Warning, "Warning!!!");
	minilog::callstackPopProc();

	minilog::deinitialize();
}

void testHTML()
{
	minilog::initialize("log.html", { .htmlLog = true });

	minilog::callstackPushProc("testHTML()->");
	minilog::log(minilog::Log, "Hello world!");
	minilog::log(minilog::Warning, "Warning!!!");
	minilog::callstackPopProc();

	minilog::deinitialize();
}

void testThread()
{
	minilog::initialize("log_thread.html", { .htmlLog = true });

	std::thread t([]()
	{
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

void testCallstack()
{
	minilog::initialize("log_callstack.txt", {});

	{
		minilog::CallstackScope scope(FUNC_NAME);

		minilog::log(minilog::Log, "Hello world!");
		minilog::log(minilog::Warning, "Warning!!!");
	}

	minilog::deinitialize();
}

int main()
{
	testTXT();
	testHTML();
	testThread();
	testCallstack();

	return 0;
}
