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

void testCallstackMacros()
{
	minilog::initialize("log_callstack_macros.txt", {});

	const unsigned int i = 32167;

	{
		minilog::CallstackScope scope(FUNC_NAME);

		LLOGL("Hello world!");
		LLOGW("Warning!!! i = %u", i);
	}

	minilog::deinitialize();
}

void testCallbacks()
{
	minilog::initialize("log_callbacks.txt", {});

	// intercept formatted messages
	minilog::LogCallback cb = { .userData = nullptr };
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

int main()
{
	testTXT();
	testHTML();
	testThread();
	testCallstack();
	testCallstackMacros();
	testCallbacks();

	return 0;
}
