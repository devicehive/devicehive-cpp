/** @file
@brief The auxiliary tools.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __HIVE_MISC_HPP_
#define __HIVE_MISC_HPP_

#include "defs.hpp"

#if !defined(HIVE_PCH)
#   include <sstream>
#   include <vector>
#endif // HIVE_PCH

namespace hive
{
    namespace misc
    {

/// @name Characters
/// @{

/// @brief Check for the simple character.
/**
@param[in] ch The input character to test.
@return `true` if input character is within range [0..127].
*/
inline bool is_char(int ch)
{
    return 0 <= ch && ch <= 127;
}


/// @brief Check for the control character.
/**
@param[in] ch The input character to test.
@return `true` if input character is within range [0..31] or is equal to 127.
*/
inline bool is_ctl(int ch)
{
    return (0 <= ch && ch <= 31) || (ch == 127);
}


/// @brief Check for the hexadecimal character.
/**
@param[in] ch The input character to test.
@return `true` if input character is within range [0-9A-Fa-f].
*/
inline bool is_hexdigit(int ch)
{
    return ('0' <= ch && ch <= '9')
        || ('A' <= ch && ch <= 'F')
        || ('a' <= ch && ch <= 'f');
}


/// @brief Check for the digital character.
/**
@param[in] ch The input character to test.
@return `true` if input character is within range [0..9].
*/
inline bool is_digit(int ch)
{
    return '0' <= ch && ch <= '9';
}


/// @brief Convert one hexadecimal character to integer.
/**
@param[in] ch The hexadecimal character.
    Should be one character from the set [0-9A-Fa-f].
@return The parsed integer or -1 in case of error.
*/
inline int hex2int(char ch)
{
    if ('0' <= ch && ch <= '9')
        return (ch - '0');
    else if ('A' <= ch && ch <= 'F')
        return (ch - 'A' + 10);
    else if ('a' <= ch && ch <= 'f')
        return (ch - 'a' + 10);

    return -1; // error
}


/// @brief Convert integer to hexadecimal character (lower case).
/**
@param[in] x The integer in range [0..16).
@return The hexadecimal character or -1 in case of error.
*/
inline char int2hex(int x)
{
    if (0 <= x && x <= 9)
        return (x + '0');
    else if (10 <= x && x <= 15)
        return (x - 10 + 'a'); // (!) lower case

    return -1; // error
}


/// @brief Convert one decimal character to integer.
/**
@param[in] ch The decimal character.
    Should be one character from the set [0-9].
@return The parsed integer or -1 in case of error.
*/
inline int dec2int(char ch)
{
    if ('0' <= ch && ch <= '9')
        return (ch - '0');

    return -1; // error
}


/// @brief Convert integer to decimal character.
/**
@param[in] x The integer in range [0..10).
@return The decimal character or -1 in case of error.
*/
inline char int2dec(int x)
{
    if (0 <= x && x <= 9)
        return (x + '0');

    return -1; // error
}

/// @}

/// @name URL encode/decode
/// @{

/// @brief URL encode (stream).
/**
This function writes the URL-encoded string to the output stream.

@param[in,out] os The output stream.
@param[in] str The string to encode.
@return The output stream.
*/
inline OStream& url_encode(OStream & os, String const& str)
{
    const size_t N = str.size();
    for (size_t i = 0; i < N; ++i)
    {
        const int ch = str[i];

        if (('0' <= ch && ch <= '9') // unreserved characters
            || ('A' <= ch && ch <= 'Z')
            || ('a' <= ch && ch <= 'z')
            || ('-' == ch) || ('_' == ch)
            || ('.' == ch) || ('~' == ch))
        {
            os.put(ch);
        }
        else
        {
            os.put('%');
            os.put(int2hex((ch>>4)&0x0f));
            os.put(int2hex(ch&0x0f));
        }
    }

    return os;
}


/// @brief URL encode.
/**
@param[in] str The string to encode.
@return The URL-encoded string.
*/
inline String url_encode(String const& str)
{
    OStringStream oss;
    url_encode(oss, str);
    return oss.str();
}


/// @brief URL decode (stream).
/**
This function writes the URL-decoded string to the output stream.

@param[in,out] os The output stream.
@param[in] str The string to decode.
@return The output stream.
@throw std::runtime_error on invalid data.
*/
inline OStream& url_decode(OStream & os, String const& str)
{
    const size_t N = str.size();
    for (size_t i = 0; i < N; ++i)
    {
        const int ch = str[i];

        if ('%' == ch) // hexadecimal value
        {
            if ((i+2) >= N)
                throw std::runtime_error("invalid url-encoded data");

            const int a = hex2int(str[i+1]);
            const int b = hex2int(str[i+2]);

            if (0 <= a && 0 <= b)
            {
                os.put((a<<4) | b);
                i += 2;
            }
            else
                throw std::runtime_error("invalid url-encoded data");
        }
        else
        {
            // no escaping
            os.put(ch);
        }
    }

    return os;
}


/// @brief URL decode.
/**
@param[in] str The string to decode.
@return The decoded string.
@throw std::runtime_error on invalid data.
*/
inline String url_decode(String const& str)
{
    OStringStream oss;
    url_decode(oss, str);
    return oss.str();
}
/// @}


/// @name Base16 encoding
/// @{

/// @brief Encode the custom binary buffer.
/**
The output buffer should be 2 times big than input buffer.

@param[in] first The begin of input buffer.
@param[in] last The end of output buffer.
@param[in] out The begin of output buffer.
@return The end of output buffer.
*/
template<typename In, typename Out>
Out base16_encode(In first, In last, Out out)
{
    while (first != last)
    {
        const int x = UInt8(*first++);

        *out++ = int2hex((x>>4)&0x0f);
        *out++ = int2hex(x&0x0f);
    }

    return out;
}


/// @brief Decode the custom binary buffer.
/**
The output buffer should be 2 times less than input buffer.

@param[in] first The begin of input buffer.
@param[in] last The end of output buffer.
@param[in] out The begin of output buffer.
@return The end of output buffer.
@throw std::runtime_error on invalid data.
*/
template<typename In, typename Out>
Out base16_decode(In first, In last, Out out)
{
    while (first != last)
    {
        const int aa =                   UInt8(*first++);
        const int bb = (first != last) ? UInt8(*first++) : 0;

        const int a = hex2int(aa);
        const int b = hex2int(bb);

        if (a < 0 || b < 0)
            throw std::runtime_error("invalid base64 data");
        *out++ = (a<<4) | b;
    }

    return out;
}


/// @brief Encode the custom binary data.
/**
@param[in] first The begin of input data.
@param[in] last The end of input data.
@return The base16 data.
*/
template<typename In> inline
String base16_encode(In first, In last)
{
    const size_t ilen = std::distance(first, last);
    const size_t olen = ilen*2;

    String buf(olen, '\0');
    buf.erase(base16_encode(first,
        last, buf.begin()), buf.end());
    return buf;
}


/// @brief Encode the custom binary vector.
/**
@param[in] data The input data.
@return The base16 data.
*/
template<typename T, typename A> inline
String base16_encode(std::vector<T,A> const& data)
{
    return base16_encode(data.begin(), data.end());
}


/// @brief Encode the custom binary string.
/**
@param[in] data The input data.
@return The base16 data.
*/
inline String base16_encode(String const& data)
{
    return base16_encode(data.begin(), data.end());
}


/// @brief Decode the base16 data.
/**
@param[in] data The base16 data.
@return The binary data.
@throw std::runtime_error on invalid data.
*/
inline std::vector<UInt8> base16_decode(String const& data)
{
    const size_t ilen = data.size();
    const size_t olen = (ilen+1)/2; // ceil(1/2*len)

    std::vector<UInt8> buf(olen);
    buf.erase(base16_decode(data.begin(),
        data.end(), buf.begin()), buf.end());
    return buf;
}

/// @}


/// @name Base64 encoding
/// @{

/// @brief Encode the custom binary buffer.
/**
The output buffer should be 4/3 times big than input buffer.

@param[in] first The begin of input buffer.
@param[in] last The end of output buffer.
@param[in] out The begin of output buffer.
@return The end of output buffer.
*/
template<typename In, typename Out>
Out base64_encode(In first, In last, Out out)
{
    const char TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

    // [xxxxxxxx][yyyyyyyy][zzzzzzzz] => [aaaaaa][bbbbbb][cccccc][dddddd]
    while (first != last)
    {
        const int x = UInt8(*first++);          // byte #0

        int a = (x>>2) & 0x3F;      // [xxxxxx--] => [00xxxxxx]
        int b = (x<<4) & 0x3F;      // [------xx] => [00xx0000]
        int c = 64;                 // '=' by default
        int d = 64;                 // '=' by default

        if (first != last)
        {
            const int y = UInt8(*first++);      // byte #1

            b |= (y>>4) & 0x0F;     // [yyyy----] => [00xxyyyy]
            c  = (y<<2) & 0x3F;     // [----yyyy] => [00yyyy00]

            if (first != last)
            {
                const int z = UInt8(*first++);   // byte #2

                c |= (z>>6) & 0x03; // [zz------] => [00yyyyzz]
                d  =  z     & 0x3F; // [--zzzzzz] => [00zzzzzz]
            }
        }

        *out++ = TABLE[a];
        *out++ = TABLE[b];
        *out++ = TABLE[c];
        *out++ = TABLE[d];
    }

    return out;
}


/// @brief Decode the custom binary buffer.
/**
The output buffer should be 4/3 times less than input buffer.

@param[in] first The begin of input buffer.
@param[in] last The end of output buffer.
@param[in] out The begin of output buffer.
@return The end of output buffer.
@throw std::runtime_error on invalid data.
*/
template<typename In, typename Out>
Out base64_decode(In first, In last, Out out)
{
    const Int8 TABLE[] =
    {
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,62,  -1,-1,-1,63,
        52,53,54,55,  56,57,58,59,  60,61,-1,-1,  -1,-1,-1,-1,
        -1, 0, 1, 2,   3, 4, 5, 6,   7, 8, 9,10,  11,12,13,14,
        15,16,17,18,  19,20,21,22,  23,24,25,-1,  -1,-1,-1,-1,
        -1,26,27,28,  29,30,31,32,  33,34,35,36,  37,38,39,40,
        41,42,43,44,  45,46,47,48,  49,50,51, 1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,
        -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1,  -1,-1,-1,-1
    };


    // [aaaaaa][bbbbbb][cccccc][dddddd] => [xxxxxxxx][yyyyyyyy][zzzzzzzz]
    // [aaaaaa][bbbbbb][cccc00][=]      => [xxxxxxxx][yyyyyyyy]
    // [aaaaaa][bb0000][=][=]           => [xxxxxxxx]
    while (first != last)
    {
        const int aa =                   UInt8(*first++);
        const int bb = (first != last) ? UInt8(*first++) : 0;
        const int cc = (first != last) ? UInt8(*first++) : 0;
        const int dd = (first != last) ? UInt8(*first++) : 0;

        const int a = TABLE[aa];
        const int b = TABLE[bb];


        if (a < 0 || b < 0)
            throw std::runtime_error("invalid base64 data");
        *out++ = (a<<2) | ((b>>4)&0x03);                // [00aaaaaa][00bb----] => [xxxxxxxx]

        if (cc != '=')
        {
            const int c = TABLE[cc];
            if (c < 0)
                throw std::runtime_error("invalid base64 data");
            *out++ = ((b<<4)&0xF0) | ((c>>2)&0x0F);     // [00--bbbb][00cccc--] => [yyyyyyyy]

            if (dd != '=')
            {
                const int d = TABLE[dd];
                if (d < 0)
                    throw std::runtime_error("invalid base64 data");
                *out++ = ((c<<6)&0xC0) | d;             // [00----cc][00dddddd] => [zzzzzzzz]
            }
            else
            {
                if (c&0x03) // sanity check
                    throw std::runtime_error("invalid base64 data");
            }
        }
        else
        {
            if ((b&0x0F) || dd != '=' || first != last) // sanity check
                throw std::runtime_error("invalid base64 data");
        }
    }

    return out;
}


/// @brief Encode the custom binary data.
/**
@param[in] first The begin of input data.
@param[in] last The end of input data.
@return The base64 data.
*/
template<typename In> inline
String base64_encode(In first, In last)
{
    const size_t ilen = std::distance(first, last);
    const size_t olen = ((ilen+2)/3)*4; // ceil(4/3*len)

    String buf(olen, '\0');
    buf.erase(base64_encode(first,
        last, buf.begin()), buf.end());
    return buf;
}


/// @brief Encode the custom binary vector.
/**
@param[in] data The input data.
@return The base64 data.
*/
template<typename T, typename A> inline
String base64_encode(std::vector<T,A> const& data)
{
    return base64_encode(data.begin(), data.end());
}


/// @brief Encode the custom binary string.
/**
@param[in] data The input data.
@return The base64 data.
*/
inline String base64_encode(String const& data)
{
    return base64_encode(data.begin(), data.end());
}


/// @brief Decode the base64 data.
/**
@param[in] data The base64 data.
@return The binary data.
@throw std::runtime_error on invalid data.
*/
inline std::vector<UInt8> base64_decode(String const& data)
{
    const size_t ilen = data.size();
    const size_t olen = ((ilen+3)/4)*3; // ceil(3/4*len)

    std::vector<UInt8> buf(olen);
    buf.erase(base64_decode(data.begin(),
        data.end(), buf.begin()), buf.end());
    return buf;
}

/// @}

    } // misc namespace
} // hive namespace

#endif // __HIVE_MISC_HPP_
