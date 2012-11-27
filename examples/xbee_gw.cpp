/** @file
@brief The test application.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_examples
*/
#include <hive/pch.hpp>

#include "xbee_gw.hpp"

#include <iostream>

#if !defined(HIVE_DISABLE_SSL)
#   if defined(_WIN32) || defined(WIN32)
#       pragma comment(lib,"ssleay32.lib")
#       pragma comment(lib,"libeay32.lib")
#   endif // WIN32
#endif // HIVE_DISABLE_SSL


/// @brief The test application entry point.
/**
@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
@return The application exit code.
*/
int main(int argc, const char *argv[])
{
    try
    {
        { // configure logging
            using namespace hive;

            log::Logger::root().setTarget(log::Target::Tie::create(
                log::Target::File::create("xbee_gw.log"),
                log::Logger::root().getTarget()));
            log::Logger::root().setLevel(log::LEVEL_TRACE);
            log::Logger("/xbee/API").setLevel(log::LEVEL_TRACE);
        }

        xbee_gw::Application::create(argc, argv)->run();
    }
    catch (std::exception const& ex)
    {
        std::cerr << "ERROR: "
            << ex.what() << "\n";
    }
    catch (...)
    {
        std::cerr << "FATAL ERROR"
            << "\n";
    }

    return 0;
}
