/** @file
@brief The test application.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_examples
*/
#include <hive/pch.hpp>

#include <examples/basic_app.hpp>
#include <examples/simple_dev.hpp>
#include <examples/simple_gw.hpp>
#include <examples/zigbee_gw.hpp>

#include "test-defs.hpp"
#include "test-swab.hpp"
#include "test-dump.hpp"
#include "test-json.hpp"
#include "test-http.hpp"
#include "test-ws13.hpp"
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

You may define XTEST_EXAMPLE macro as basic_app, simple_dev,
simple_gw, etc. to run exactly that example code.

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
#if defined(XTEST_EXAMPLE)  // custom example
        XTEST_EXAMPLE::main(argc, argv);
#elif defined(XTEST_UNIT)   // custom unit-test
        XTEST_UNIT();
#else                       // manual
        if (0) basic_app::main(argc, argv);
        if (0) simple_dev::main(argc, argv, false); // RESTful
        if (0) simple_dev::main(argc, argv, true);  // WebSocket
        if (1) simple_gw::main(argc, argv, false);  // RESTful
        if (0) simple_gw::main(argc, argv, true);   // WebSocket
        if (0) zigbee_gw::main(argc, argv, false);  // RESTful
        if (0) zigbee_gw::main(argc, argv, true);   // WebSocket

        if (0) test_defs0();
        if (0) test_swab0();
        if (0) test_dump0();
        if (0) test_json0();
        if (0) test_json1(1<argc ? argv[1] : "../json");
        if (0) test_http0();
        if (0) test_http1();
        if (0) test_ws13_0();
        if (0) test_ws13_1();
        if (0) test_log_0();
        if (0) test_log_1();
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
