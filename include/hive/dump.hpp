/** @file
@brief The various dump tools.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_dump
*/
#ifndef __HIVE_DUMP_HPP_
#define __HIVE_DUMP_HPP_

#include "defs.hpp"
#include "misc.hpp"

#if !defined(HIVE_PCH)
#   include <sstream>
#   include <vector>
#endif // HIVE_PCH


namespace hive
{
    /// @brief The various dump tools.
    /**
    This namespace contains various tools to dump data in *HEX* or *ASCII* formats.
    */
    namespace dump
    {

/// @name Dump containers to HEX
/// @{

/// @brief Dump a binary data to an output stream in *HEX* format.
/**
@param[in,out] os The output stream.
@param[in] first The begin of the binary data.
@param[in] last The end of the binary data.
@return The output stream.
*/
template<typename In>
OStream& hex(OStream &os, In first, In last)
{
    for (; first != last; ++first)
    {
        const unsigned int b = int(*first); // one byte
        os.put(misc::int2hex((b>>4)&0x0F)); // high nibble
        os.put(misc::int2hex((b>>0)&0x0F)); // low nibble
    }

    return os;
}


/// @brief Dump a binary data to a string in *HEX* format.
/**
@param[in] first The begin of the binary data.
@param[in] last The end of the binary data.
@return The dump in *HEX* format.
*/
template<typename In> inline
String hex(In first, In last)
{
    OStringStream oss;
    hex(oss, first, last);
    return oss.str();
}


/// @brief Dump a binary vector to an output stream in *HEX* format.
/**
@param[in,out] os The output stream.
@param[in] data The binary data.
@return The output stream.
*/
template<typename T, typename A> inline
OStream& hex(OStream &os, std::vector<T,A> const& data)
{
    return hex(os, data.begin(), data.end());
}


/// @brief Dump a binary vector to a string in *HEX* format.
/**
@param[in] data The binary data.
@return The dump in *HEX* format.
*/
template<typename T, typename A> inline
String hex(std::vector<T,A> const& data)
{
    return hex(data.begin(), data.end());
}


/// @brief Dump a binary string to an output stream in *HEX* format.
/**
@param[in,out] os The output stream.
@param[in] data The binary data.
@return The output stream.
*/
inline OStream& hex(OStream &os, String const& data)
{
    return hex(os, data.begin(), data.end());
}


/// @brief Dump a binary string to a string in *HEX* format.
/**
@param[in] data The binary data.
@return The dump in *HEX* format.
*/
inline String hex(String const& data)
{
    return hex(data.begin(), data.end());
}

/// @}


        /// @brief The implementation specific stuff.
        namespace impl
        {

/// @brief Dump an integer to an output stream in *HEX* format.
/**
@param[in,out] os The output stream.
@param[in] x The integer to dump.
@return The output stream.
*/
template<typename IntX>
OStream& int2hex(OStream &os, IntX x)
{
    const size_t N = 2*sizeof(IntX);
    for (size_t i = 0; i < N; ++i)
    {
        const int nibble = int(x >> ((N-1-i)*4));
        os.put(misc::int2hex(nibble&0x0F));
    }

    return os;
}


/// @brief Dump an integer to a string in *HEX* format.
/**
@param[in] x The integer to dump.
@return The dump in *HEX* format.
*/
template<typename IntX>
String int2hex(IntX x)
{
    const size_t N = 2*sizeof(IntX);

    String s;
    s.reserve(N);

    for (size_t i = 0; i < N; ++i)
    {
        const int nibble = int(x >> ((N-1-i)*4));
        s.push_back(misc::int2hex(nibble&0x0F));
    }

    return s;
}

        } // helpers


/// @name Dump integers to HEX
/// @{

/// @brief Dump a 8-bits integer to a string in *HEX* format.
/**
@param[in] x The integer.
@return The dump in *HEX* format.
*/
inline String hex(UInt8 x)
{
    return impl::int2hex(x);
}

/// @copydoc hex(UInt8)
inline String hex(Int8 x)
{
    return impl::int2hex(x);
}


/// @brief Dump a 16-bits integer to a string in *HEX* format.
/**
@param[in] x The integer.
@return The dump in *HEX* format.
*/
inline String hex(UInt16 x)
{
    return impl::int2hex(x);
}

/// @copydoc hex(UInt16)
inline String hex(Int16 x)
{
    return impl::int2hex(x);
}


/// @brief Dump a 32-bits integer to a string in *HEX* format.
/**
@param[in] x The integer.
@return The dump in *HEX* format.
*/
inline String hex(UInt32 x)
{
    return impl::int2hex(x);
}

/// @copydoc hex(UInt32)
inline String hex(Int32 x)
{
    return impl::int2hex(x);
}


/// @brief Dump a 64-bits integer to a string in *HEX* format.
/**
@param[in] x The integer.
@return The dump in *HEX* format.
*/
inline String hex(UInt64 x)
{
    return impl::int2hex(x);
}

/// @copydoc hex(UInt64)
inline String hex(Int64 x)
{
    return impl::int2hex(x);
}

/// @}
    } // HEX


    // ASCII
    namespace dump
    {

/// @name Dump containers to ASCII
/// @{

/// @brief Dump a binary data to an output stream in *ASCII* format.
/**
@param[in,out] os The output stream.
@param[in] first The begin of the binary data.
@param[in] last The end of the binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The output stream.
*/
template<typename In>
OStream& ascii(OStream &os, In first, In last, int bad = '.')
{
    for (; first != last; ++first)
    {
        const int b = int(*first); // one byte
        if (misc::is_char(b) && !misc::is_ctl(b))
            os.put(b);
        else
            os.put(bad);
    }

    return os;
}


/// @brief Dump a binary data to a string in *ASCII* format.
/**
@param[in] first The begin of the binary data.
@param[in] last The end of the binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The dump in *ASCII* format.
*/
template<typename In> inline
String ascii(In first, In last, int bad = '.')
{
    OStringStream oss;
    ascii(oss, first, last, bad);
    return oss.str();
}


/// @brief Dump a binary vector to an output stream in *ASCII* format.
/**
@param[in,out] os The output stream.
@param[in] data The binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The output stream.
*/
template<typename T, typename A> inline
OStream& ascii(OStream &os, std::vector<T,A> const& data, int bad = '.')
{
    return ascii(os, data.begin(), data.end(), bad);
}


/// @brief Dump a binary vector to a string in *ASCII* format.
/**
@param[in] data The binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The dump in *ASCII* format.
*/
template<typename T, typename A> inline
String ascii(std::vector<T,A> const& data, int bad = '.')
{
    return ascii(data.begin(), data.end(), bad);
}


/// @brief Dump a binary string to an output stream in *ASCII* format.
/**
@param[in,out] os The output stream.
@param[in] data The binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The output stream.
*/
inline OStream& ascii(OStream &os, String const& data, int bad = '.')
{
    return ascii(os, data.begin(), data.end(), bad);
}


/// @brief Dump a binary string to a string in *ASCII* format.
/**
@param[in] data The binary data.
@param[in] bad The replacement for non-ASCII characters.
@return The dump in *ASCII* format.
*/
inline String ascii(String const& data, int bad = '.')
{
    return ascii(data.begin(), data.end(), bad);
}

/// @}
    } // dump namespace
} // hive namespace

#endif // __HIVE_DUMP_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_dump Dump tools

This tools are mostly used for debugging and logging purposes.
It's possible to dump any binary container or integer.

There are two kind of dump functions:
  - hive::dump::hex() functions are used to dump in *HEX* format.
  - hive::dump::ascii() functions are used to dump in *ASCII* format.

In *HEX* format all binary bytes are replaced with two hexadecimal digit (lower case).

*ASCII* format uses characters in range [32..127). Any other characters
are replaced with `bad` placeholder which is '.' by default.
*/
