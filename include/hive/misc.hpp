/** @file
@brief The auxiliary tools.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __HIVE_MISC_HPP_
#define __HIVE_MISC_HPP_

#include "defs.hpp"

namespace hive
{
    namespace misc
    {

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
@return `true` if input character is within range ['0'..'9'].
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

    } // misc namespace
} // hive namespace

#endif // __HIVE_MISC_HPP_
