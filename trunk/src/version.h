#if defined(_DEBUG)
#define ONSLAUGHT_BUILD_VERSION 99999999
#define ONSLAUGHT_BUILD_VERSION_WSTR L"Debug"
#define ONSLAUGHT_BUILD_VERSION_STR "Debug"
#elif defined(NONS_SVN)
#define ONSLAUGHT_BUILD_VERSION 20101121
#define ONSLAUGHT_BUILD_VERSION_WSTR L"SVN 2010-11-21 11:57:34"
#define ONSLAUGHT_BUILD_VERSION_STR "SVN 2010-11-21 11:57:34"
#else
#define ONSLAUGHT_BUILD_VERSION 20101121
#define ONSLAUGHT_BUILD_VERSION_WSTR L"Beta 2010-11-21 11:57:34"
#define ONSLAUGHT_BUILD_VERSION_STR "Beta 2010-11-21 11:57:34"
#endif
#define ONSLAUGHT_COPYRIGHT_YEAR_STR "2008-2010"
#define ONSLAUGHT_COPYRIGHT_YEAR_WSTR L"2008-2010"
