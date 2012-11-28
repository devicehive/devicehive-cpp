/** @file
@brief The test application.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_examples
*/
#include <hive/pch.hpp>

#include "test-defs.hpp"
#include "test-swab.hpp"
#include "test-dump.hpp"
#include "test-json.hpp"
#include "test-http.hpp"
#include "test-log.hpp"

#include <iostream>

#if !defined(HIVE_DISABLE_SSL)
#   if defined(_WIN32) || defined(WIN32)
#       pragma comment(lib,"ssleay32.lib")
#       pragma comment(lib,"libeay32.lib")
#   endif // WIN32
#endif // HIVE_DISABLE_SSL


/// @brief The test application entry point.
/**
This function is used for debug & unit-test purposes.

Also you may define XTEST_UNIT as test_defs0, test_swab0, etc
to run exactly that unit test code.

This macro may be used from Makefile to build
custom unit test from one source file.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
@return The application exit code.
*/
int main(int argc, const char *argv[])
{
    try
    {
#if defined(XTEST_UNIT) // custom unit-test
        XTEST_UNIT();
#else                   // manual
        if (0) test_defs0();
        if (0) test_swab0();
        if (0) test_dump0();
        if (0) test_json0();
        if (1) test_json1(1<argc ? argv[1] : "../json");
        if (0) test_http0();
        if (0) test_http1();
        if (0) test_log0();
        if (0) test_log1();
#endif // XTEST_UNIT
    }
    catch (std::exception const& ex)
    {
        std::cerr << "ERROR: "
            << ex.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "FATAL ERROR\n";
    }

    return 0;
}