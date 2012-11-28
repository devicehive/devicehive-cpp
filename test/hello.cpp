/** @file
@brief HelloWorld application.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

Application just prints the "Hello"
and all its command line arguments.
*/
#include <iostream>

/// @brief The application entry point.
/**
@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
@return The application exit code.
*/
int main(int argc, const char* argv[])
{
    std::cout << "Hello";
    for (int i = 1; i < argc; ++i)
        std::cout << " " << argv[i];
    std::cout << "!\n";

    return 0;
}
