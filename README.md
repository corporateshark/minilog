# 🪵 minilog ![MIT](https://img.shields.io/badge/license-MIT-blue.svg)  [![GitHub](https://img.shields.io/badge/repo-github-green.svg)](https://github.com/corporateshark/minilog)
Minimalistic logging library with threads and manual callstacks.

## Features

* Easy to use (2 files, C-style interface)
* Multiple outputs (a text log file, an HTML log file, Android logcat, system console with colors)
* Thread-safe (write to log from multiple threads, set thread names)
* Cross-platform (Windows, Linux, OS X, Android)
* Callbacks (if you want to intercept formatted log messages)
* C++14

## Basic usage

```
#include "minilog/minilog.h"
...
minilog::initialize("log.txt", {});
minilog::log(minilog::Log, "Hello world!");
minilog::log(minilog::Log, "%s", "Hello again!");
minilog::log(minilog::Warning, "Warning!!!");
minilog::deinitialize();
```

Console output:

![image](https://user-images.githubusercontent.com/2510143/139719447-50c48b77-9f56-41d5-b1c3-0b98000f652d.png)

## HTML log + manual callstack + multiple threads

All calls to `log()`, `callstackPushProc()`, `callstackPopProc()` are thread-safe.

```
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
```

HTML output:

![image](https://user-images.githubusercontent.com/2510143/139718798-5536413a-72ff-49c0-a262-1c1bb844a260.png)

You can also use the `CallstackScope` class to manage your callstack in RAII-style.
