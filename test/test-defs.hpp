/** @file
@brief The definitions unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/defs.hpp>
#include <stdexcept>
#include <iostream>
#include <assert.h>

namespace
{
    using namespace hive;

// assert macro, throws exception
#define MY_ASSERT(cond, msg) \
    assert(cond && msg);     \
    if (cond) {} else throw std::runtime_error(msg)

// test application entry point
/*
Checks for integers size.
*/
void test_defs0()
{
    std::cout << "check for integer sizes... ";

    MY_ASSERT(1==sizeof(UInt8) && 1==sizeof(Int8), "invalid UInt8/Int8 size");
    MY_ASSERT(2==sizeof(UInt16) && 2==sizeof(Int16), "invalid UInt16/Int16 size");
    MY_ASSERT(4==sizeof(UInt32) && 4==sizeof(Int32), "invalid UInt32/Int32 size");
    MY_ASSERT(8==sizeof(UInt64) && 8==sizeof(Int64), "invalid UInt64/Int64 size");

    std::cout << "done\n";
}

#undef MY_ASSERT

} // local namespace
