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
    if (cond) {} else throw std::runtime_error(msg)


template<typename T>
void check_dump_hex()
{
    // check for random generated integers
    for (size_t i = 0; i < 1000; ++i)
    {
        T val = T(0); // reference value
        for (size_t k = 0; k < sizeof(T); ++k)
        {
            val <<= 8;
            val |= rand()&0xFF;
        }

        OStringStream oss;
        oss << std::hex;
        oss.fill('0');

        oss << std::setw(2*sizeof(T))
            << val;

        MY_ASSERT(dump::hex(val) == oss.str(),
            "invalid dump::hex(int)");
    }
}


// test application entry point
/*
Checks for auxiliary dump tools.
*/
void test_dump0()
{
    std::cout << "check for dump::hex... ";
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

    //check_dump_hex<UInt8>();  check_dump_hex<Int8>();
    check_dump_hex<UInt16>(); check_dump_hex<Int16>();
    check_dump_hex<UInt32>(); check_dump_hex<Int32>();
    check_dump_hex<UInt64>(); check_dump_hex<Int64>();

    std::cout << "done\n\n";
}

#undef MY_ASSERT

} // local namespace
