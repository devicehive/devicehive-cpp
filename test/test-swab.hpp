/** @file
@brief The byte order unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/swab.hpp>
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

template<typename T>
void check_swab()
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

        union TmpBuf
        {
            char buf[sizeof(T)];
            T val;
        };

        volatile TmpBuf LE, BE;
        LE.val = misc::h2le(val);
        BE.val = misc::h2be(val);

        for (size_t k = 0; k < sizeof(T); ++k)
        {
            const char b = (val >> (k*8))&0xFF;
            MY_ASSERT(LE.buf[k] == b, "invalid LE byte order");
            MY_ASSERT(BE.buf[sizeof(T)-k-1] == b, "invalid BE byte order");
        }

        MY_ASSERT(misc::le2h(misc::h2le(val)) == val,
            "h2le and le2h should be consistent");
        MY_ASSERT(misc::be2h(misc::h2be(val)) == val,
            "h2be and be2h should be consistent");
    }
}

// test application entry point
/*
Checks for swab functions.
*/
void test_swab0()
{
    std::cout << "check for byte order changing... ";

    check_swab<UInt16>(); check_swab<Int16>();
    check_swab<UInt32>(); check_swab<Int32>();
    check_swab<UInt64>(); check_swab<Int64>();

    std::cout << "done\n";
}

#undef MY_ASSERT

} // local namespace
