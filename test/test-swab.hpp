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
    union TypeConv
    {
        T    val;
        char buf[sizeof(T)];
    };

    MY_ASSERT(sizeof(TypeConv) == sizeof(T),
        "invalid type size");

    for (size_t i = 0; i < 1000; ++i)
    {
        TypeConv aa;
        for (size_t k = 0; k < sizeof(T); ++k)
            aa.buf[k] = rand()&0xFF;

        TypeConv bb;
        bb.val = misc::swab(aa.val); // convert
        for (size_t k = 0; k < sizeof(T); ++k)
        {
            MY_ASSERT(aa.buf[k] == bb.buf[sizeof(T)-k-1],
                "invalid byte order");
        }
    }
}

// test application entry point
/*
Checks for swab functions.
*/
void test_swab0()
{
    std::cout << "check for byte order changing... ";

    check_T<UInt16>();
    check_T<UInt32>();
    check_T<UInt64>();

    std::cout << "done\n";
}

#undef MY_ASSERT

} // local namespace
