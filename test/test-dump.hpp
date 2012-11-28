/** @file
@brief The auxiliary dump tools unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/dump.hpp>
#include <stdexcept>
#include <iostream>
#include <assert.h>

namespace
{
    using namespace hive;

// assert macro, throws exception
#define MY_ASSERT(cond, msg) \
    std::cout << "checking: " << #cond << "\n"; \
    if (cond) {} else throw std::runtime_error(msg)


// test application entry point
/*
Checks for auxiliary dump tools.
*/
void test_dump0()
{
    { // check dump functions
        std::vector<int> d;
        d.push_back(0);
        d.push_back(1);
        d.push_back(127);
        d.push_back(255);
        d.push_back(-1);

        //dump::hex(std::cout, d.begin(), d.end());
        MY_ASSERT("00017fffff" == dump::hex(d) && "d={0, 1, 127, 255, -1}", "invalid HEX dump");
        MY_ASSERT("....." == dump::ascii(d) && "d={0, 1, 127, 255, -1}", "invalid ASCII dump");
    }

    std::cout << "\n\n";
}

#undef MY_ASSERT

} // local namespace
