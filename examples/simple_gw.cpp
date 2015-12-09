/** @file
@brief The simple gateway application.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_examples
*/
#include <hive/pch.hpp>

#include "simple_gw.hpp"

#include <iostream>

#if !defined(HIVE_DISABLE_SSL)
#   if defined(_MSC_VER) && (defined(_WIN32) || defined(WIN32))
#       pragma comment(lib,"ssleay32.lib")
#       pragma comment(lib,"libeay32.lib")
#   endif // WIN32
#endif // HIVE_DISABLE_SSL


/// @brief The simple gateway application entry point.
/**
@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
@return The application exit code.
*/
int main(int argc, const char *argv[])
{
    try
    {
        using namespace examples;
        simple_gw::main(argc, argv);
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
