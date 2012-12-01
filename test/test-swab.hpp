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
void check_T()
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

        volatile char LE[sizeof(T)];
        volatile char BE[sizeof(T)];

        *((T*)LE) = misc::h2le(val);
        *((T*)BE) = misc::h2be(val);

        for (size_t k = 0; k < sizeof(T); ++k)
        {
            const char b = (val >> (k*8))&0xFF;
            MY_ASSERT(LE[k] == b, "invalid LE byte order");
            MY_ASSERT(BE[sizeof(T)-k-1] == b, "invalid BE byte order");
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

    check_T<UInt16>(); check_T<Int16>();
    check_T<UInt32>(); check_T<Int32>();
    check_T<UInt64>(); check_T<Int64>();

    std::cout << "done\n";
}

#undef MY_ASSERT

} // local namespace
