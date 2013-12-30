/** @file
@brief The byte order wrappers.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

Defines #HIVE_LITTLE_ENDIAN or #HIVE_BIG_ENDIAN macro to
indicate little-endian or big-endian platform.

@see @ref page_hive_swab
*/
#ifndef __HIVE_SWAB_HPP_
#define __HIVE_SWAB_HPP_

#include "defs.hpp"

// defines BOOST_BIG_ENDIAN or BOOST_LITTLE_ENDIAN
// also defines BOOST_BYTE_ORDER as 4321 or 1234
#include <boost/detail/endian.hpp>
#if defined(BOOST_LITTLE_ENDIAN)
#   define HIVE_LITTLE_ENDIAN 1
#   define HIVE_BYTE_ORDER 1234
#elif defined(BOOST_BIG_ENDIAN)
#   define HIVE_BIG_ENDIAN 1
#   define HIVE_BYTE_ORDER 4321
#else
#   error Unknown architecture
#endif


#if defined(HIVE_DOXY_MODE)
/// @brief The little-endian platform indicator.
#define HIVE_LITTLE_ENDIAN

/// @brief The big-endian platform indicator.
#define HIVE_BIG_ENDIAN

/// @brief The current byte order indicator.
/**
Defined as:
- `1234` for little-endian platforms
- `4321` for big-endian platforms
*/
#define HIVE_BYTE_ORDER
#endif // HIVE_DOXY_MODE


#if defined(WIN32) || defined(_WIN32) // win
#   include <stdlib.h>
#elif defined(__APPLE__)              //osx                                                                                                                                                                  
#   include <machine/endian.h>
#   include <libkern/OSByteOrder.h>
#else                                 // nix
#   include <endian.h>
#endif // WIN32


namespace hive
{
    // swab functions
    namespace misc
    {

/// @name Change byte order
/// @{

/// @brief Reverse byte order (16-bits).
/**
@param[in] x The 16-bits integer.
@return The reversed 16-bits integer.
*/
inline UInt16 swab_16(UInt16 x)
{
#if defined(WIN32) || defined(_WIN32)  // win
    return _byteswap_ushort(x);
#elif defined(__APPLE__)               // osx                                                                                                                                                                
    // GCC 4.6 lacks __builtin_bswap16                                                                                                                           
#if defined(__clang__) && (__GNUC__ >=4 && __GNUC_MINOR__ > 6)
    return __builtin_bswap16(x);
#else
    return ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8);
#endif
#else                                  // nix                                                                                                                                                                
    return __bswap_16(x);
#endif
}


/// @brief Reverse byte order (32-bits).                                                                                                                                                                     
/**                                                                                                                                                                                                          
@param[in] x The 32-bits integer.                                                                                                                                                                            
@return The reversed 32-bits integer.                                                                                                                                                                        
*/
inline UInt32 swab_32(UInt32 x)
{
#if defined(WIN32) || defined(_WIN32)  // win                                                                                                                                                                
    return _byteswap_ulong(x);
#elif defined(__APPLE__)               // osx                                                                                                                                                                
    return __builtin_bswap32(x);
#else                                  // nix                                                                                                                                                                
    return __bswap_32(x);
#endif
}


/// @brief Reverse byte order (64-bits).                                                                                                                                                                     
/**                                                                                                                                                                                                          
@param[in] x The 64-bits integer.                                                                                                                                                                            
@return The reversed 64-bits integer.                                                                                                                                                                        
*/
inline UInt64 swab_64(UInt64 x)
{
#if defined(WIN32) || defined(_WIN32)  // win                                                                                                                                                                
    return _byteswap_uint64(x);
#elif defined(__APPLE__)               // osx                                                                                                                                                                
    return __builtin_bswap64(x);
#else                                  // nix                                                                                                                                                                
    return __bswap_64(x);
#endif
}

/// @copydoc swab_16()
inline UInt16 swab(UInt16 x)
{
    return swab_16(x);
}

/// @copydoc swab_16()
inline Int16 swab(Int16 x)
{
    return swab_16(x);
}


/// @copydoc swab_32()
inline UInt32 swab(UInt32 x)
{
    return swab_32(x);
}

/// @copydoc swab_32()
inline Int32 swab(Int32 x)
{
    return swab_32(x);
}


/// @copydoc swab_64()
inline UInt64 swab(UInt64 x)
{
    return swab_64(x);
}

/// @copydoc swab_64()
inline Int64 swab(Int64 x)
{
    return swab_64(x);
}

/// @}

    } // swab functions


    // big-endian
    namespace misc
    {

/// @name Host and big-endian
/// @{

/// @brief Convert host to big-endian (16-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 16-bits integer in host format.
@return The 16-bits integer in big-endian format.
*/
inline UInt16 h2be_16(UInt16 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_16(x);
#else                           // big-endian
    return x;
#endif
}


/// @brief Convert big-endian to host (16-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 16-bits integer in big-endian format.
@return The 16-bits integer in host format.
*/
inline UInt16 be2h_16(UInt16 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_16(x);
#else                           // big-endian
    return x;
#endif
}


/// @brief Convert host to big-endian (32-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 32-bits integer in host format.
@return The 32-bits integer in big-endian format.
*/
inline UInt32 h2be_32(UInt32 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_32(x);
#else                           // big-endian
    return x;
#endif
}


/// @brief Convert big-endian to host (32-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 32-bits integer in big-endian format.
@return The 32-bits integer in host format.
*/
inline UInt32 be2h_32(UInt32 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_32(x);
#else                           // big-endian
    return x;
#endif
}


/// @brief Convert host to big-endian (64-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 64-bits integer in host format.
@return The 64-bits integer in big-endian format.
*/
inline UInt64 h2be_64(UInt64 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_64(x);
#else                           // big-endian
    return x;
#endif
}


/// @brief Convert big-endian to host (64-bits).
/**
Does nothing on big-endian platforms.

@param[in] x The 64-bits integer in big-endian format.
@return The 64-bits integer in host format.
*/
inline UInt64 be2h_64(UInt64 x)
{
#if defined(HIVE_LITTLE_ENDIAN) // little-endian
    return swab_64(x);
#else                           // big-endian
    return x;
#endif
}


/// @copydoc h2be_16()
inline UInt16 h2be(UInt16 x)
{
    return h2be_16(x);
}

/// @copydoc be2h_16()
inline UInt16 be2h(UInt16 x)
{
    return be2h_16(x);
}

/// @copydoc h2be_16()
inline Int16 h2be(Int16 x)
{
    return h2be_16(x);
}

/// @copydoc be2h_16()
inline Int16 be2h(Int16 x)
{
    return be2h_16(x);
}


/// @copydoc h2be_32()
inline UInt32 h2be(UInt32 x)
{
    return h2be_32(x);
}

/// @copydoc be2h_32()
inline UInt32 be2h(UInt32 x)
{
    return be2h_32(x);
}

/// @copydoc h2be_32()
inline Int32 h2be(Int32 x)
{
    return h2be_32(x);
}

/// @copydoc be2h_32()
inline Int32 be2h(Int32 x)
{
    return be2h_32(x);
}


/// @copydoc h2be_64()
inline UInt64 h2be(UInt64 x)
{
    return h2be_64(x);
}

/// @copydoc be2h_64()
inline UInt64 be2h(UInt64 x)
{
    return be2h_64(x);
}

/// @copydoc h2be_64()
inline Int64 h2be(Int64 x)
{
    return h2be_64(x);
}

/// @copydoc be2h_64()
inline Int64 be2h(Int64 x)
{
    return be2h_64(x);
}

/// @}

    } // big-endian


    // little-endian
    namespace misc
    {

/// @name Host and little-endian
/// @{

/// @brief Convert host to little-endian (16-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 16-bits integer in host format.
@return The 16-bits integer in little-endian format.
*/
inline UInt16 h2le_16(UInt16 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_16(x);
#else                           // little-endian
    return x;
#endif
}


/// @brief Convert little-endian to host (16-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 16-bits integer in little-endian format.
@return The 16-bits integer in host format.
*/
inline UInt16 le2h_16(UInt16 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_16(x);
#else                           // little-endian
    return x;
#endif
}


/// @brief Convert host to little-endian (32-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 32-bits integer in host format.
@return The 32-bits integer in little-endian format.
*/
inline UInt32 h2le_32(UInt32 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_32(x);
#else                           // little-endian
    return x;
#endif
}


/// @brief Convert little-endian to host (32-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 32-bits integer in little-endian format.
@return The 32-bits integer in host format.
*/
inline UInt32 le2h_32(UInt32 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_32(x);
#else                           // little-endian
    return x;
#endif
}


/// @brief Convert host to little-endian (64-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 64-bits integer in host format.
@return The 64-bits integer in little-endian format.
*/
inline UInt64 h2le_64(UInt64 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_64(x);
#else                           // little-endian
    return x;
#endif
}


/// @brief Convert little-endian to host (64-bits).
/**
Does nothing on little-endian platforms.

@param[in] x The 64-bits integer in little-endian format.
@return The 64-bits integer in host format.
*/
inline UInt64 le2h_64(UInt64 x)
{
#if defined(HIVE_BIG_ENDIAN)    // big-endian
    return swab_64(x);
#else                           // little-endian
    return x;
#endif
}



/// @copydoc h2le_16()
inline UInt16 h2le(UInt16 x)
{
    return h2le_16(x);
}

/// @copydoc le2h_16()
inline UInt16 le2h(UInt16 x)
{
    return le2h_16(x);
}

/// @copydoc h2le_16()
inline Int16 h2le(Int16 x)
{
    return h2le_16(x);
}

/// @copydoc le2h_16()
inline Int16 le2h(Int16 x)
{
    return le2h_16(x);
}


/// @copydoc h2le_32()
inline UInt32 h2le(UInt32 x)
{
    return h2le_32(x);
}

/// @copydoc le2h_32()
inline UInt32 le2h(UInt32 x)
{
    return le2h_32(x);
}

/// @copydoc h2le_32()
inline Int32 h2le(Int32 x)
{
    return h2le_32(x);
}

/// @copydoc le2h_32()
inline Int32 le2h(Int32 x)
{
    return le2h_32(x);
}


/// @copydoc h2le_64()
inline UInt64 h2le(UInt64 x)
{
    return h2le_64(x);
}

/// @copydoc le2h_64()
inline UInt64 le2h(UInt64 x)
{
    return le2h_64(x);
}

/// @copydoc h2le_64()
inline Int64 h2le(Int64 x)
{
    return h2le_64(x);
}

/// @copydoc le2h_64()
inline Int64 le2h(Int64 x)
{
    return le2h_64(x);
}

/// @}
    } // little-endian


    // converters
    namespace misc
    {

/// @name Byte order converters
/// @{

/// @brief The endian converter prototype.
/**
There is no actual implementation.

Use static methods e2h() and h2e() of the following template specializations:
    - EndianT<1234> for little-endian
    - EndianT<4321> for big-endian

@see hive::misc::LittleEndian
@see hive::misc::BigEndian
*/
template<int>
class EndianT
{};


/// @brief The little-endian converter.
/**
This class is used to convert little-endian integers
to the host and visa versa.

Please use the h2e() static methods to convert integers
from host to little-endian and e2h() static methods to
convert integers from little-endian to host.
*/
template<>
class EndianT<1234>
{
public:

    /// @copydoc h2le_16()
    static inline UInt16 h2e(UInt16 x)
    {
        return h2le_16(x);
    }

    /// @copydoc le2h_16()
    static inline UInt16 e2h(UInt16 x)
    {
        return le2h_16(x);
    }

    /// @copydoc h2le_16()
    static inline Int16 h2e(Int16 x)
    {
        return h2le_16(x);
    }

    /// @copydoc le2h_16()
    static inline Int16 e2h(Int16 x)
    {
        return le2h_16(x);
    }

public:

    /// @copydoc h2le_32()
    static inline UInt32 h2e(UInt32 x)
    {
        return h2le_32(x);
    }

    /// @copydoc le2h_32()
    static inline UInt32 e2h(UInt32 x)
    {
        return le2h_32(x);
    }

    /// @copydoc h2le_32()
    static inline Int32 h2e(Int32 x)
    {
        return h2le_32(x);
    }

    /// @copydoc le2h_32()
    static inline Int32 e2h(Int32 x)
    {
        return le2h_32(x);
    }

public:

    /// @copydoc h2le_64()
    static inline UInt64 h2e(UInt64 x)
    {
        return h2le_64(x);
    }

    /// @copydoc le2h_64()
    static inline UInt64 e2h(UInt64 x)
    {
        return le2h_64(x);
    }

    /// @copydoc h2le_64()
    static inline Int64 h2e(Int64 x)
    {
        return h2le_64(x);
    }

    /// @copydoc le2h_64()
    static inline Int64 e2h(Int64 x)
    {
        return le2h_64(x);
    }
};


/// @brief The big-endian converter.
/**
This class is used to convert big-endian integers
to the host and visa versa.

Please use the h2e() static methods to convert integers
from host to big-endian and e2h() static methods to
convert integers from big-endian to host.
*/
template<>
class EndianT<4321>
{
public:

    /// @copydoc h2be_16()
    static inline UInt16 h2e(UInt16 x)
    {
        return h2be_16(x);
    }

    /// @copydoc be2h_16()
    static inline UInt16 e2h(UInt16 x)
    {
        return be2h_16(x);
    }

    /// @copydoc h2be_16()
    static inline Int16 h2e(Int16 x)
    {
        return h2be_16(x);
    }

    /// @copydoc be2h_16()
    static inline Int16 e2h(Int16 x)
    {
        return be2h_16(x);
    }

public:

    /// @copydoc h2be_32()
    static inline UInt32 h2e(UInt32 x)
    {
        return h2be_32(x);
    }

    /// @copydoc be2h_32()
    static inline UInt32 e2h(UInt32 x)
    {
        return be2h_32(x);
    }

    /// @copydoc h2be_32()
    static inline Int32 h2e(Int32 x)
    {
        return h2be_32(x);
    }

    /// @copydoc be2h_32()
    static inline Int32 e2h(Int32 x)
    {
        return be2h_32(x);
    }

public:

    /// @copydoc h2be_64()
    static inline UInt64 h2e(UInt64 x)
    {
        return h2be_64(x);
    }

    /// @copydoc be2h_64()
    static inline UInt64 e2h(UInt64 x)
    {
        return be2h_64(x);
    }

    /// @copydoc h2be_64()
    static inline Int64 h2e(Int64 x)
    {
        return h2be_64(x);
    }

    /// @copydoc be2h_64()
    static inline Int64 e2h(Int64 x)
    {
        return be2h_64(x);
    }
};


/// @brief The little-endian converter type.
/**
@see EndianT<1234>
*/
typedef EndianT<1234> LittleEndian;


/// @brief The big-endian converter type.
/**
@see EndianT<4321>
*/
typedef EndianT<4321> BigEndian;

    } // converters
} // hive namespace

#endif // __HIVE_SWAB_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_swab Byte order tools

There are two platform types: *little-endian* (LE) and *big-endian* (BE).
They are differ in way to save/load multibyte integers to/from memory.
For example, the hexadecimal `AABBCCDD` value (which is `2864434397` decimal)
stored in memory in the following ways:

| # |  LE  |  BE  |                        |
|---|------|------|------------------------|
| 0 | `AA` | `DD` | lowest memory address  |
| 1 | `BB` | `CC` |                        |
| 2 | `CC` | `BB` |                        |
| 3 | `DD` | `AA` | highest memory address |

Most of Intel processors are little-endian. Some processors like ARM and MIPS
support both modes. In network protocols big-endian format is preferable.


Platform byte order macro
-------------------------

This header file defines the following macro which might be used to byte-order
specific code:
- #HIVE_LITTLE_ENDIAN is defined on little-endian platforms
- #HIVE_BIG_ENDIAN is defined on big-endian platforms
- #HIVE_BYTE_ORDER is defined as `1234` or `4321` on little-endian
  and big-endian platforms respectively.


Change the byte order
---------------------

There are a few functions to change the byte order:
- hive::misc::swab_16() for 16-bits integers
- hive::misc::swab_32() for 32-bits integers
- hive::misc::swab_64() for 64 bits integers
- hive::misc::swab() overloaded for all integer sizes

The signed integers might be safely converted too.


Little/big-endian and host
--------------------------

There are a few functions to convert little-endian and big-endian integers
to the host byte order.

The following functions are used to convert little-endian integers
to the host byte order and visa versa:
- hive::misc::le2h_16(), hive::misc::h2le_16()
- hive::misc::le2h_32(), hive::misc::h2le_32()
- hive::misc::le2h_64(), hive::misc::h2le_64()
- hive::misc::le2h(), hive::misc::h2le()

The following functions are used to convert big-endian integers
to the host byte order and visa versa:
- hive::misc::be2h_16(), hive::misc::h2be_16()
- hive::misc::be2h_32(), hive::misc::h2be_32()
- hive::misc::be2h_64(), hive::misc::h2be_64()
- hive::misc::be2h(), hive::misc::h2be()

Both signed and unsigned integers may be converted.


Byte order converters
---------------------

The hive::misc::LittleEndian and hive::misc::BigEndian classes can be used as
little-endian and big-endian converters. See their static methods documentation.
*/
