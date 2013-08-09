/** @file
@brief The JSON classes.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

@see @ref page_hive_json
*/
#ifndef __HIVE_JSON_HPP_
#define __HIVE_JSON_HPP_

#include "defs.hpp"
#include "misc.hpp"

#if !defined(HIVE_PCH)
#   include <assert.h>
#   include <sstream>
#   include <string>
#   include <vector>
#   include <limits>
#   include <map>
#endif // HIVE_PCH

// TODO: use boost::multi_index_container for JSON objects!

// extension: single quoted strings
#if !defined(HIVE_JSON_SINGLE_QUOTED_STRING)
#   define HIVE_JSON_SINGLE_QUOTED_STRING   1
#endif // HIVE_JSON_SINGLE_QUOTED_STRING

// extension: simple strings [0-9A-Za-z_]
#if !defined(HIVE_JSON_SIMPLE_STRING)
#   define HIVE_JSON_SIMPLE_STRING          1
#endif // HIVE_JSON_SIMPLE_STRING


namespace hive
{
    /// @brief The JSON module.
    /**
    This namespace contains classes and functions related to JSON data processing.
    @see @ref page_hive_json
    */
    namespace json
    {
        /// @brief The JSON exceptions.
        /**
        This namespace contains JSON exceptions.
        */
        namespace error
        {

/// @brief The basic JSON exception.
/**
This is base class for all exceptions used in JSON module.
*/
class Failure:
    public std::runtime_error
{
    /// @brief The base type.
    typedef std::runtime_error Base;

public:

    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit Failure(const char *message)
        : Base(message)
    {}


    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit Failure(String const& message)
        : Base(message.c_str())
    {}


    /// @brief The trivial destructor.
    virtual ~Failure() throw()
    {}
};


/// @brief The type cast exception.
/**
This exception is thrown when JSON value cannot
be cast to specified type.
*/
class CastError:
    public Failure
{
    /// @brief The base type.
    typedef Failure Base;

public:

    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit CastError(const char *message)
        : Base(message)
    {}


    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit CastError(String const& message)
        : Base(message)
    {}


    /// @brief The trivial destructor.
    virtual ~CastError() throw()
    {}
};


/// @brief The syntax exception.
/**
This exception may be thrown during JSON parsing.
*/
class SyntaxError:
    public Failure
{
    /// @brief The base type.
    typedef Failure Base;

public:

    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit SyntaxError(const char *message)
        : Base(message)
    {}


    /// @brief The main constructor.
    /**
    @param[in] message The error message.
    */
    explicit SyntaxError(String const& message)
        : Base(message)
    {}


    /// @brief The trivial destructor.
    virtual ~SyntaxError() throw()
    {}
};

        } // error namespace


/// @brief The JSON value.
/**
The JSON value is a variant type, which may be one of the following:
  - **NULL**
  - **BOOLEAN**
  - **INTEGER**
  - **DOUBLE**
  - **STRING**
  - **ARRAY**
  - **OBJECT**

UNDER CONSTRUCTION!
-------------------

There are some automatic conversions:
  - **NULL** to **ARRAY**
  - **NULL** to **OBJECT**

| type          | type check    | conversion check          | conversion    |
|---------------|---------------|---------------------------|---------------|
| **NULL**      | isNull()      |                           |               |
| **BOOLEAN**   | isBool()      | isConvertibleToBool()     | asBool()      |
| **INTEGER**   | isInteger()   | isConvertibleToInteger()  | asInt()       |
| **DOUBLE**    | isDouble()    | isConvertibleToDouble()   | asDouble()    |
| **STRING**    | isString()    | isConvertibleToString()   | asString()    |
| **ARRAY**     | isArray()     |                           |               |
| **OBJECT**    | isObject()    |                           |               |

There are also a lot of fixed-size integer conversions (with value range check):

| integer   | unsigned                  | signed                |
|-----------|---------------------------|-----------------------|
| 64-bits   | asUInt64() \n asUInt()    | asInt64() \n asInt()  |
| 32-bits   | asUInt32()                | asInt32()             |
| 16-bits   | asUInt16()                | asInt16()             |
| 8-bits    | asUInt8()                 | asInt8()              |
*/
class Value
{
public:

    /// @brief The value type.
    enum Type
    {
        TYPE_NULL,    ///< @brief The **NULL** value type.
        TYPE_BOOLEAN, ///< @brief The **BOOLEAN** value type.
        TYPE_INTEGER, ///< @brief The **INTEGER** value type.
        TYPE_DOUBLE,  ///< @brief The **DOUBLE** value type.
        TYPE_STRING,  ///< @brief The **STRING** value type.
        TYPE_ARRAY,   ///< @brief The **ARRAY** value type.
        TYPE_OBJECT   ///< @brief The **OBJECT** value type.
    };

#if 1
/// @name Main constructors
/// @{
public:

    /// @brief The default constructor.
    /**
    This constructor creates the **NULL** value by default.
    But it's possible to construct default value of any type,
    especially empty **ARRAY** or **OBJECT** values.
    @param[in] type The value type.
    */
    explicit Value(Type type = TYPE_NULL)
        : m_type(type)
    {
        m_val.i = 0; // avoid garbage
    }


    /// @brief Construct the **BOOLEAN** value.
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(bool val)
        : m_type(TYPE_BOOLEAN)
    {
        m_val.i = val?1:0;
    }


    /// @brief Construct the **INTEGER** value.
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(Int64 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **DOUBLE** value.
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(double val)
        : m_type(TYPE_DOUBLE)
    {
        m_val.f = val;
    }


    /// @brief Construct the **STRING** value.
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(String const& val)
        : m_type(TYPE_STRING)
        , m_str(val)
    {
        m_val.i = 0; // avoid garbage
    }
/// @}
#endif // main constructors

#if 1
/// @name Auxiliary constructors
/// @{
public:

    /// @brief Construct the **INTEGER** value (unsigned 64 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(UInt64 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
        // TODO: check for signed overflow (m_val.i < 0)?
    }


    /// @brief Construct the **INTEGER** value (unsigned 32 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(UInt32 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **INTEGER** value (signed 32 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(Int32 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **INTEGER** value (unsigned 16 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(UInt16 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **INTEGER** value (signed 16 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(Int16 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **INTEGER** value (unsigned 8 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(UInt8 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **INTEGER** value (signed 8 bits).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(Int8 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the **DOUBLE** value (float).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(float val)
        : m_type(TYPE_DOUBLE)
    {
        m_val.f = val;
    }


    /// @brief Construct the **STRING** value (C-string).
    /**
    @param[in] val The value.
    */
    /*explicit*/ Value(const char* val)
        : m_type(TYPE_STRING)
        , m_str(val)
    {
        m_val.i = 0; // avoid garbage
    }
/// @}
#endif // auxiliary constructors

#if 1
/// @name Copy, move and swap
/// @{
public:

    /// @brief The copy-constructor.
    /**
    @param[in] other The other value to copy.
    */
    Value(Value const& other)
        : m_type(other.m_type)
        , m_val(other.m_val)
        , m_str(other.m_str)
        , m_arr(other.m_arr)
        , m_obj(other.m_obj)
    {}


    /// @brief The copy assignment.
    /**
    @param[in] other The other value to copy.
    @return The self reference.
    */
    Value& operator=(Value const& other)
    {
        Value(other).swap(*this);
        return *this;
    }


    /// @brief Swap the two values.
    /**
    Swap content of two values.

    @param[in,out] other The value to swap.
    */
    void swap(Value &other)
    {
        std::swap(m_type, other.m_type);
        std::swap(m_val, other.m_val);
        std::swap(m_str, other.m_str);
        std::swap(m_arr, other.m_arr);
        std::swap(m_obj, other.m_obj);
    }

#if defined(HIVE_HAS_RVALUE_REFS)

    /// @brief The move-constructor.
    /**
    @param[in] other The other value to move.
    */
    Value(Value && other)
        : m_type(other.m_type)
        , m_val(other.m_val)
        , m_str(std::move(other.m_str))
        , m_arr(std::move(other.m_arr))
        , m_obj(std::move(other.m_obj))
    {}


    /// @brief The move assignment.
    /**
    @param[in] other The other value to copy.
    @return The self reference.
    */
    Value& operator=(Value && other)
    {
        if (this != &other)
            swap(other);
        return *this;
    }

#endif // defined(HIVE_HAS_RVALUE_REFS)
/// @}
#endif // copy, move and swap

public:

    /// @brief Get the **NULL** value.
    /**
    This static value is used to indicate the **NULL** values.
    @return The static **NULL** value reference.
    */
    static Value const& null()
    {
        static Value N(TYPE_NULL);
        return N;
    }

#if 1
/// @name Type checks
/// @{
public:

    /// @brief Get the value type.
    /**
    @return The value type.
    */
    Type getType() const
    {
        return m_type;
    }


    /// @brief Is the value **NULL**?
    /**
    @return `true` if the value is **NULL**.
    */
    bool isNull() const
    {
        return (TYPE_NULL == m_type);
    }


    /// @copydoc isNull()
    bool operator!() const
    {
        return isNull();
    }


    /// @brief Is the value **BOOLEAN**?
    /**
    @return `true` if the value is **BOOLEAN**.
    */
    bool isBool() const
    {
        return (TYPE_BOOLEAN == m_type);
    }


    /// @brief Is the value **INTEGER**?
    /**
    @return `true` if the value is **INTEGER**.
    */
    bool isInteger() const
    {
        return (TYPE_INTEGER == m_type);
    }


    /// @brief Is the value **DOUBLE**?
    /**
    @return `true` if the value if **DOUBLE**.
    */
    bool isDouble() const
    {
        return (TYPE_DOUBLE == m_type);
    }


    /// @brief Is the value **STRING**?
    /**
    @return `true` if the value is **STRING**.
    */
    bool isString() const
    {
        return (TYPE_STRING == m_type);
    }


    /// @brief Is the value **ARRAY**?
    /**
    @return `true` if the value is **ARRAY**.
    */
    bool isArray() const
    {
        return (TYPE_ARRAY == m_type);
    }


    /// @brief Is the value **OBJECT**?
    /**
    @return `true` if the value is **OBJECT**.
    */
    bool isObject() const
    {
        return (TYPE_OBJECT == m_type);
    }
/// @}
#endif // type checks

#if 1
/// @name Main conversions
/// @{
public:

    /// @brief Is the value convertible to **BOOLEAN**?
    /**
    @return `true` if the value is convertible.
    @see asBool()
    */
    bool isConvertibleToBool() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
            case TYPE_DOUBLE:
                return true;

            case TYPE_STRING:
                return m_str.empty()
                    || m_str=="false"
                    || m_str=="true";

            default: // no conversion
                return false;
        }
    }


    /// @brief Get the value as **BOOLEAN**.
    /**
    The conversion rules are the following:

    | current type | conversion                                                                 |
    |--------------|----------------------------------------------------------------------------|
    |    **NULL**  | always `false`                                                             |
    |  **BOOLEAN** | as is                                                                      |
    |  **INTEGER** | zero as `false`, non-zero as `true`                                        |
    |  **DOUBLE**  | zero as `false`, non-zero as `true`                                        |
    |  **STRING**  | "true" as `true` \n "false"  or "" as `false` \n otherwise no conversion   |
    |   **ARRAY**  | no conversion                                                              |
    |  **OBJECT**  | no conversion                                                              |

    @return The **BOOLEAN** value.
    @throw error::CastError if no conversion available.
    */
    bool asBool() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return false; // NULL as false

            case TYPE_BOOLEAN:
            case TYPE_INTEGER: // true as non-zero
                return (0 != m_val.i);

            case TYPE_DOUBLE: // true as non-zero
                return (0.0 != m_val.f); // exactly!

            case TYPE_STRING:
                if (!m_str.empty())
                {
                    if (m_str == "true")
                        return true;
                    else if (m_str == "false")
                        return false;
                    else
                        break; // will throw
                    // TODO: maybe convert to integer?
                }
                else
                    return false; // false if empty

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to boolean");
    }

public:

    /// @brief Is the value convertible to **INTEGER**?
    /**
    @return `true` if the value is convertible.
    @see asInt()
    */
    bool isConvertibleToInteger() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
            case TYPE_DOUBLE:
                return true;

            case TYPE_STRING:
                if (!m_str.empty())
                {
                    IStringStream iss(m_str);
                    Int64 tmp_val = 0;
                    return ((iss >> tmp_val)
                        && (iss >> std::ws).eof());
                }
                else
                    return true; // empty as zero

            default: // no conversion
                return false;
        }
    }


    /// @brief Get the value as **INTEGER**.
    /**
    The conversion rules are the following:

    | current type | conversion                                                                 |
    |--------------|----------------------------------------------------------------------------|
    |    **NULL**  | always `0`                                                                 |
    |  **BOOLEAN** | `false` as `0`, `true` as `1`                                              |
    |  **INTEGER** | as is                                                                      |
    |  **DOUBLE**  | rounded to lowest!                                                         |
    |  **STRING**  | empty string as `0` \n parsed integer if valid \n otherwise no conversion  |
    |   **ARRAY**  | no conversion                                                              |
    |  **OBJECT**  | no conversion                                                              |

    @return The **INTEGER** value.
    @throw error::CastError if no conversion available.
    */
    Int64 asInteger() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return 0; // NULL as zero

            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
                return m_val.i;

            case TYPE_DOUBLE:
                return Int64(m_val.f); // floor

            case TYPE_STRING:
            {
                if (!m_str.empty())
                {
                    IStringStream iss(m_str);
                    Int64 val = 0;
                    if ((iss >> val) && (iss >> std::ws).eof())
                        return val;
                    // else no conversion
                }
                else
                    return 0; // empty as zero
            } break;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to integer");
    }

public:

    /// @brief Is the value convertible to **DOUBLE**?
    /**
    @return `true` if the value is convertible.
    @see asDouble()
    */
    bool isConvertibleToDouble() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
            case TYPE_DOUBLE:
                return true;

            case TYPE_STRING:
                if (!m_str.empty())
                {
                    IStringStream iss(m_str);
                    double tmp_val = 0.0;
                    return ((iss >> tmp_val)
                        && (iss >> std::ws).eof());
                }
                else
                    return true; // empty as zero

            default: // no conversion
                return false;
        }
    }


    /// @brief Get the value as **DOUBLE**.
    /**
    The conversion rules are the following:

    | current type | conversion                                                                 |
    |--------------|----------------------------------------------------------------------------|
    |    **NULL**  | always `0.0`                                                               |
    |  **BOOLEAN** | `false` as `0.0`, `true` as `1.0`                                          |
    |  **INTEGER** | as is                                                                      |
    |  **DOUBLE**  | as is                                                                      |
    |  **STRING**  | empty string as `0.0` \n parsed double if valid \n otherwise no conversion |
    |   **ARRAY**  | no conversion                                                              |
    |  **OBJECT**  | no conversion                                                              |

    @return The **DOUBLE** value.
    @throw error::CastError if no conversion available.
    */
    double asDouble() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return 0.0; // NULL as zero

            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
                return double(m_val.i);

            case TYPE_DOUBLE:
                return m_val.f;

            case TYPE_STRING:
            {
                if (!m_str.empty())
                {
                    IStringStream iss(m_str);
                    double val = 0.0;
                    if ((iss >> val) && (iss >> std::ws).eof())
                        return val;
                    // else no conversion
                }
                else
                    return 0.0; // empty as zero
            } break;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to double");
    }

public:

    /// @brief Is the value convertible to **STRING**?
    /**
    @return `true` if the value is convertible.
    @see asString()
    */
    bool isConvertibleToString() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
            case TYPE_DOUBLE:
            case TYPE_STRING:
                return true;

            default: // no conversion
                return false;
        }
    }


    /// @brief Get the value as **STRING**.
    /**
    The conversion rules are the following:

    | current type | conversion                            |
    |--------------|---------------------------------------|
    |    **NULL**  | always empty string                   |
    |  **BOOLEAN** | `false` as "false", `true` as "true"  |
    |  **INTEGER** | as `printf("%d")`                     |
    |  **DOUBLE**  | as `printf("%g")`                     |
    |  **STRING**  | as is                                 |
    |   **ARRAY**  | no conversion                         |
    |  **OBJECT**  | no conversion                         |

    @return The **STRING** value.
    @throw error::CastError if no conversion available.
    */
    String asString() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return String(); // NULL as empty string

            case TYPE_BOOLEAN:
                return m_val.i ? "true" : "false";

            case TYPE_INTEGER:
            {
                OStringStream oss;
                oss << m_val.i;
                return oss.str();
            }

            case TYPE_DOUBLE:
            {
                OStringStream oss;
                oss << m_val.f;
                return oss.str();
            }

            case TYPE_STRING:
                return m_str;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to string");
    }

public:

    /// @brief Is the value convertible to the other type?
    /**
    @param[in] type The type convert to.
    @return `true` if the value is convertible.
    */
    bool isConvertibleTo(Type type) const
    {
        switch (type)
        {
            case TYPE_NULL:     return isNull();
            case TYPE_BOOLEAN:  return isConvertibleToBool();
            case TYPE_INTEGER:  return isConvertibleToInteger();
            case TYPE_DOUBLE:   return isConvertibleToDouble();
            case TYPE_STRING:   return isConvertibleToString();
            case TYPE_ARRAY:    return isNull() || isArray();
            case TYPE_OBJECT:   return isNull() || isObject();
        }

        return false; // no conversion
    }

/// @}
#endif // Main conversions

#if 1
/// @name Auxiliary conversions
/// @{
public:

    /// @copydoc asUInt64()
    UInt64 asUInt() const
    {
        return asUInt64();
    }


    /// @copydoc asInt64()
    Int64 asInt() const
    {
        return asInt64();
    }


    /// @brief Get the value as unsigned **INTEGER** (64 bits).
    /**
    @return The unsigned **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    UInt64 asUInt64() const
    {
        const Int64 val = asInteger();
        if (val < 0)
            throw error::CastError("is negative");
        return val;
    }


    /// @brief Get the value as signed **INTEGER** (64 bits).
    /**
    @return The signed **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    Int64 asInt64() const
    {
        return asInteger();
    }


    /// @brief Get the value as unsigned **INTEGER** (32 bits).
    /**
    @return The unsigned **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    UInt32 asUInt32() const
    {
        const Int64 val = asInteger();
        const UInt32 res = UInt32(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }


    /// @brief Get the value as signed **INTEGER** (32 bits).
    /**
    @return The signed **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    Int32 asInt32() const
    {
        const Int64 val = asInteger();
        const Int32 res = Int32(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }


    /// @brief Get the value as unsigned **INTEGER** (16 bits).
    /**
    @return The unsigned **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    UInt16 asUInt16() const
    {
        const Int64 val = asInteger();
        const UInt16 res = UInt16(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }


    /// @brief Get the value as signed **INTEGER** (16 bits).
    /**
    @return The signed **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    Int16 asInt16() const
    {
        const Int64 val = asInteger();
        const Int16 res = Int16(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }


    /// @brief Get the value as unsigned **INTEGER** (8 bits).
    /**
    @return The unsigned **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    UInt8 asUInt8() const
    {
        const Int64 val = asInteger();
        const UInt8 res = UInt8(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }


    /// @brief Get the value as signed **INTEGER** (8 bits).
    /**
    @return The signed **INTEGER** value.
    @throw error::CastError if no conversion available.
    @see asInteger()
    */
    Int8 asInt8() const
    {
        const Int64 val = asInteger();
        const Int8 res = Int8(val);
        if (val != Int64(res))
            throw error::CastError("out of range");
        return res;
    }

/// @}
#endif // auxiliary conversions

public:

    /// @brief Is equal to the other value?
    /**
    The two values are equal if they have the same type
    and exactly the same content.

    @param[in] other The other value to compare.
    @return `true` if value is equal to the other.
    */
    bool equal(Value const& other) const
    {
        // different types
        if (m_type != other.m_type)
            return false;

        // the same type
        switch (m_type)
        {
            case TYPE_NULL:
                return true;

            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
                return m_val.i == other.m_val.i;

            case TYPE_DOUBLE:
                return m_val.f == other.m_val.f;

            case TYPE_STRING:
                return m_str == other.m_str;

            case TYPE_ARRAY:
                return m_arr.size() == other.m_arr.size()
                    && std::equal(m_arr.begin(), m_arr.end(),
                            other.m_arr.begin());

            case TYPE_OBJECT:
                // TODO: compare as unordered set!
                return m_obj.size() == other.m_obj.size()
                    && std::equal(m_obj.begin(), m_obj.end(),
                            other.m_obj.begin());

            default:
                return false;
        }
    }

public: // array & object

    /// @brief Get the number of elements in **ARRAY** or **OBJECT**.
    /**
    For all other types returns zero.
    @return The number of array elements for **ARRAY**
        or the number of members for **OBJECT**.
    */
    size_t size() const
    {
        switch (m_type)
        {
            //case TYPE_STRING:
            //    return m_str.size();

            case TYPE_ARRAY:
                return m_arr.size();

            case TYPE_OBJECT:
                return m_obj.size();

            default:
                return 0;
        }
    }


    /// @brief Is the **ARRAY** or **OBJECT** empty?
    /**
    @return `true` if **ARRAY** is empty, **OBJECT** is empty, or value is **NULL**.
    */
    bool empty() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return true;

            //case TYPE_STRING:
            //    return m_str.empty();

            case TYPE_ARRAY:
                return m_arr.empty();

            case TYPE_OBJECT:
                return m_obj.empty();

            default:
                return false;
        }
    }


    /// @brief Remove all **OBJECT** members or **ARRAY** elements.
    void clear()
    {
        switch (m_type)
        {
            //case TYPE_STRING:
            //    m_str.clear();
            //    break;

            case TYPE_ARRAY:
                m_arr.clear();
                break;

            case TYPE_OBJECT:
                m_obj.clear();
                break;

            default:
                break;
        }
    }

#if 1
/// @name Array
/// @{
public:

    /// @brief Resize the **ARRAY**.
    /**
    This method changes value type to **ARRAY** if current type is **NULL**.

    New array elements are initialized by @a def value, **NULL** by default.

    @param[in] size The new array size.
    @param[in] def The default value to fill array.
    */
    void resize(size_t size, Value const& def = Value())
    {
        assert((isNull() || isArray())
            && "not an array");
        if (m_type != TYPE_ARRAY)
            m_type = TYPE_ARRAY;
        m_arr.resize(size, def);
    }


    /// @brief Get an **ARRAY** element.
    /**
    @param[in] index The zero-based element index.
    @return The array element reference.
    */
    Value& operator[](size_t index)
    {
        assert(isArray() && "not an array");
        assert(index < m_arr.size()
            && "index out of range");
        return m_arr[index];
    }


    /// @brief Get an **ARRAY** element (read-only).
    /**
    @param[in] index The zero-based element index.
    @return The array element reference.
    */
    Value const& operator[](size_t index) const
    {
        assert(isArray() && "not an array");
        assert(index < m_arr.size()
            && "index out of range");
        return m_arr[index];
    }


    /// @brief Append value to **ARRAY** at the end.
    /**
    This method changes value type to **ARRAY** if current type is **NULL**.

    @param[in] val The value to append.
    */
    void append(Value const& val)
    {
        assert((isNull() || isArray())
            && "not an array");
        if (m_type != TYPE_ARRAY)
            m_type = TYPE_ARRAY;
        m_arr.push_back(val);
    }


    /// @brief The **ARRAY** elements iterator type.
    /**
    This type is used to iterate all elements in **ARRAY**.
    */
    typedef std::vector<Value>::const_iterator ElementIterator;


    /// @brief Get the begin of **ARRAY**.
    /**
    @return The begin of **ARRAY**.
    */
    ElementIterator elementsBegin() const
    {
        assert((isNull() || isArray())
            && "not an array");
        return m_arr.begin();
    }


    /// @brief Get the end of **ARRAY**.
    /**
    @return The end of **ARRAY**.
    */
    ElementIterator elementsEnd() const
    {
        assert((isNull() || isArray())
            && "not an array");
        return m_arr.end();
    }

private:

    /// @brief The internal **ARRAY** type.
    typedef std::vector<Value> Array;

/// @}
#endif // array

#if 1
/// @name Object
/// @{
public:

    /// @brief Get the **OBJECT** member by name.
    /**
    @param[in] name The member name.
    @param[in] def The default value if no member with such name exists.
    @return The member value or @a def if no member with such name exists.
    */
    Value const& get(String const& name, Value const& def) const
    {
        assert((isNull() || isObject()) && "not an object");
        Object::const_iterator m = findMember(name);
        return (m == m_obj.end()) ? def : m->second;
    }


    /// @brief Get the member by name or create new one.
    /**
    This method changes value type to **OBJECT** if current type is **NULL**.

    **NULL** member value created if no member with such name exists.

    @param[in] name The member name.
    @return The member value.
    */
    Value& get(String const& name)
    {
        assert((isNull() || isObject())
            && "not an object");

        Object::iterator m = findMember(name);
        if (m == m_obj.end())
        {
            if (TYPE_NULL == m_type)
                m_type = TYPE_OBJECT;
            m = m_obj.insert(m, std::make_pair(name, Value()));
        }

        return m->second;
    }


    /// @brief Get the **OBJECT** member by name.
    /**
    @param[in] name The member name.
    @return The member value or null().
    */
    Value const& operator[](String const& name) const
    {
        return get(name, null());
    }


    /// @brief Get the **OBJECT** member by name or create new.
    /**
    **NULL** member value created if no member with such name exists.

    @param[in] name The member name.
    @return The member value.
    */
    Value& operator[](String const& name)
    {
        return get(name);
    }


    /// @brief Is **OBJECT** member value exists?
    /**
    @param[in] name The member name.
    @return `true` if member with such name exists.
    */
    bool hasMemeber(String const& name) const
    {
        assert((isNull() || isObject()) && "not an object");
        Object::const_iterator m = findMember(name);
        return (m != m_obj.end());
    }


    /// @brief Remove **OBJECT** member.
    /**
    @param[in] name The member name to remove.
    */
    void removeMember(String const& name)
    {
        assert((isNull() || isObject()) && "not an object");
        Object::iterator m = findMember(name);
        if (m != m_obj.end())
            m_obj.erase(m);
    }


    /// @brief The **OBJECT** members iterator type.
    /**
    This type is used to iterate all member names on **OBJECT**.
    */
    typedef std::vector< std::pair<String,Value> >::const_iterator MemberIterator;


    /// @brief Get the begin of **OBJECT** members.
    /**
    @return The begin of **OBJECT** members.
    */
    MemberIterator membersBegin() const
    {
        assert((isNull() || isObject())
            && "not an object");
        return m_obj.begin();
    }


    /// @brief Get the end of **OBJECT** members.
    /**
    @return The end of **OBJECT** members.
    */
    MemberIterator membersEnd() const
    {
        assert((isNull() || isObject())
            && "not an object");
        return m_obj.end();
    }

private:

    /// @brief The internal **OBJECT** type.
    typedef std::vector< std::pair<String,Value> > Object;


    /// @brief Find **OBJECT** member by name.
    /**
    @param[in] name The memeber name.
    @return The member iterator.
    */
    Object::const_iterator findMember(String const& name) const
    {
        const Object::const_iterator e = m_obj.end();
        for (Object::const_iterator i = m_obj.begin(); i != e; ++i)
        {
            if (i->first == name)
                return i;
        }

        return e; // not found
    }


    /// @brief Find **OBJECT** member by name.
    /**
    @param[in] name The memeber name.
    @return The member iterator.
    */
    Object::iterator findMember(String const& name)
    {
        const Object::iterator e = m_obj.end();
        for (Object::iterator i = m_obj.begin(); i != e; ++i)
        {
            if (i->first == name)
                return i;
        }

        return e; // not found
    }

/// @}
#endif // object

private:
    Type m_type; ///< @brief The value type.

    /// @brief The %POD data holder type.
    union POD
    {
        double f; ///< @brief The floating-point value.
         Int64 i; ///< @brief The signed integer value.
    } m_val; ///< @brief The %POD data holder.

    // TODO: move these to union as pointers?
    String m_str; ///< @brief The **STRING** value.
    Array  m_arr; ///< @brief The **ARRAY** value.
    Object m_obj; ///< @brief The **OBJECT** value.
};


/// @brief Are two JSON values equal?
/** @relates Value
@param[in] a The first JSON value.
@param[in] b The second JSON value.
@return `true` if two JSON values are equal.
@see Value::equal()
*/
inline bool operator==(Value const& a, Value const& b)
{
    return a.equal(b);
}


/// @brief Are two JSON values different?
/** @relates Value
@param[in] a The first JSON value.
@param[in] b The second JSON value.
@return `true` if two JSON values are different.
@see Value::equal()
*/
inline bool operator!=(Value const& a, Value const& b)
{
    return !a.equal(b);
}


/// @brief The JSON formatter.
/**
This class writes JSON value to an output stream.

There are two formats available:
  - *human-friendly* - with spaces, new lines and indents
  - *simple* - without any spaces and new lines
*/
class Formatter
{
public:

    /// @brief Write JSON value to an output stream.
    /**
    Both formats *simple* and *human-fiendly* are supported.

    @param[in,out] os The output stream.
    @param[in] jval The JSON value to write.
    @param[in] humanFriendly The *human-fiendly* format flag.
    @param[in] indent The first line indent.
        Is used for *human-friendly* format.
    @param[in] escaping The *escaping* string flag.
    @return The output stream.
    */
    static OStream& write(OStream &os, Value const& jval,
        bool humanFriendly, size_t indent = 0, bool escaping = true)
    {
        switch (jval.getType())
        {
            case Value::TYPE_NULL:
                os << "null";
                break;

            case Value::TYPE_BOOLEAN:
                os << (jval.asBool() ? "true" : "false");
                break;

            case Value::TYPE_INTEGER:
                os << jval.asInt();
                break;

            case Value::TYPE_DOUBLE:
                os << jval.asDouble();
                break;

            case Value::TYPE_STRING:
                writeString(os, jval.asString(), escaping, true);
                break;

            case Value::TYPE_ARRAY:
            {
                os << '[';
                if (const size_t N = jval.size())
                {
                    for (size_t i = 0; i < N; ++i)
                    {
                        if (0 < i)
                            os.put(',');
                        if (humanFriendly)
                        {
                            os.put('\n');
                            writeIndent(os,
                                indent+1);
                        }
                        write(os, jval[i],
                            humanFriendly,
                            indent + 1,
                            escaping);
                    }
                    if (humanFriendly)
                    {
                        os.put('\n');
                        writeIndent(os,
                            indent);
                    }
                }
                os << ']';
            } break;

            case Value::TYPE_OBJECT:
            {
                os << '{';
                const Value::MemberIterator b = jval.membersBegin();
                const Value::MemberIterator e = jval.membersEnd();
                if (b != e)
                {
                    for (Value::MemberIterator i = b; i != e; ++i)
                    {
                        const String &name = i->first;
                        const Value &val = i->second;

                        if (i != b)
                            os.put(',');
                        if (humanFriendly)
                        {
                            os.put('\n');
                            writeIndent(os,
                                indent+1);
                        }
                        writeString(os, name, escaping, false);
                        os.put(':');
                        if (humanFriendly)
                            os.put(' ');
                        write(os, val,
                            humanFriendly,
                            indent + 1,
                            escaping);
                    }
                    if (humanFriendly)
                    {
                        os.put('\n');
                        writeIndent(os,
                            indent);
                    }
                }
                os << '}';
            } break;
        }

        return os;
    }

public:

    /// @brief Write indent to an output stream.
    /**
    This method writes the `2*indent` spaces to the output stream.

    @param[in,out] os The output stream.
    @param[in] indent The indent size.
    @return The output stream.
    */
    static OStream& writeIndent(OStream &os, size_t indent)
    {
        const size_t N = 2*indent;
        for (size_t i = 0; i < N; ++i)
            os.put(' ');
        return os;
    }


    /// @brief Write quoted/unquoted string to an output stream.
    /**
    This method writes the string in queted or unquoted format.
    All special and UNICODE characters are escaped.

    @param[in,out] os The output stream.
    @param[in] str The string to write.
    @param[in] escaping The *escaping* string flag.
    @param[in] forceQuote Force to use quotes flag.
    @return The output stream.
    */
    static OStream& writeString(OStream &os, String const& str, bool escaping, bool forceQuote)
    {
        if (escaping || !isSimple(str))
            return writeQuotedString(os, str);
        else  if (forceQuote || str.empty())
            return os << "\"" << str << "\"";
        else
            return os << str;
    }


    /// @brief Check if a character is simple.
    /**
    @param[in] ch The character to check.
    @return `true` if character is in range [0-9A-Za-z_].
    */
    static bool isSimple(int ch)
    {
        return ('0' <= ch && ch <= '9')
            || ('A' <= ch && ch <= 'Z')
            || ('a' <= ch && ch <= 'z')
            || ('_' == ch);
    }


    /// @brief Check if a string is simple.
    /**
    @param[in] str The string the check.
    @return `true` if string contains simple characters only.
    */
    static bool isSimple(String const& str)
    {
        const size_t N = str.size();
        for (size_t i = 0; i < N; ++i)
        {
            const int ch = (unsigned char)(str[i]);
            // TODO: handle utf-8 strings!!!

            if (!isSimple(ch))
                return false;
        }

        return true; // simple string
    }


    /// @brief Write quoted string to an output stream.
    /**
    This method writes the string in queted format.
    All special and UNICODE characters are escaped.

    @param[in,out] os The output stream.
    @param[in] str The string to write.
    @return The output stream.
    */
    static OStream& writeQuotedString(OStream &os, String const& str)
    {
        os.put('\"');

        const size_t N = str.size();
        for (size_t i = 0; i < N; ++i)
        {
            const int ch = (unsigned char)(str[i]);
            // TODO: handle utf-8 strings!!!

            switch (ch)
            {
                case '\"': case '\'':
                case '\\': case '/':
                    os.put('\\');
                    os.put(ch);
                    break;

                case '\b':
                    os.put('\\');
                    os.put('b');
                    break;

                case '\f':
                    os.put('\\');
                    os.put('f');
                    break;

                case '\n':
                    os.put('\\');
                    os.put('n');
                    break;

                case '\r':
                    os.put('\\');
                    os.put('r');
                    break;

                case '\t':
                    os.put('\\');
                    os.put('t');
                    break;

                default:
                    if (!misc::is_char(ch) || misc::is_ctl(ch))
                    {
                        os.put('\\');
                        os.put('u');
                        os.put(misc::int2hex((ch>>12)&0x0F));
                        os.put(misc::int2hex((ch>>8)&0x0F));
                        os.put(misc::int2hex((ch>>4)&0x0F));
                        os.put(misc::int2hex((ch>>0)&0x0F));
                    }
                    else
                        os.put(ch);
                    break;
            }
        }

        os.put('\"');
        return os;
    }
};


/// @brief Write JSON value to an output stream.
/** @relates Value
@param[in,out] os The output stream.
@param[in] jval The JSON value.
@return The output stream.
@see Formatter
*/
inline OStream& operator<<(OStream &os, const Value &jval)
{
    const bool humanFriendly = false; // true;
    return Formatter::write(os, jval, humanFriendly);
}


/// @brief Convert JSON value to string.
/** @relates Value
@param[in] jval The JSON value.
@return The JSON string.
*/
inline String toStr(Value const& jval)
{
    OStringStream oss;
    Formatter::write(oss, jval, false);
    return oss.str();
}


/// @brief Convert JSON value to *human-friendly* string.
/** @relates Value
@param[in] jval The JSON value.
@return The JSON string.
*/
inline String toStrH(Value const& jval)
{
    OStringStream oss;
    Formatter::write(oss, jval, true);
    return oss.str();
}


/// @brief Convert JSON value to *human-friendly* string without escaping.
/** @relates Value
@param[in] jval The JSON value.
@return The JSON string.
*/
inline String toStrHH(Value const& jval)
{
    OStringStream oss;
    Formatter::write(oss, jval, true, 0, false);
    return oss.str();
}


/// @brief The JSON parser.
/**
This class parses JSON values from an input stream.
Exception is thrown then input stream doesn't contain a valid JSON value.

The following comment styles are supported:
  - bash style: `#` skip all until the end of line
  - C++ style: `//` skip all until the end of line
  - C style: `/``*` skip all until the surrounding `*``/`
*/
class Parser
{
    /// @brief The string traits type.
    typedef String::traits_type Traits;
public:

    /// @brief Parse the JSON value from an input stream.
    /**
    This method parses the first JSON value from the input stream.
    All comments are ignored.

    @param[in,out] is The input stream.
    @param[out] jval The parsed JSON value.
    @return The input stream.
    @throw error::SyntaxError in case of parsing error.
    */
    static IStream& parse(IStream &is, Value &jval)
    {
        skipCommentsAndWS(is);
        Traits::int_type meta = is.peek();
        if (Traits::eq_int_type(meta, Traits::eof()))
            throw error::SyntaxError("no JSON value"); // not enough data

        Traits::char_type cx = Traits::to_char_type(meta);

        // object
        if (Traits::eq(cx, '{'))
        {
            is.ignore(1); // ignore '{'
            Value(Value::TYPE_OBJECT).swap(jval);
            bool firstMember = true;

            while (is)
            {
                skipCommentsAndWS(is);

                cx = Traits::to_char_type(is.peek());
                if (Traits::eq(cx, '}'))
                {
                    is.ignore(1); // ignore '}'
                    break; // end of object
                }

                if (!firstMember) // check member separator
                {
                    if (Traits::eq(cx, ','))
                    {
                        is.ignore(1); // ingore ','
                        skipCommentsAndWS(is);
                    }
                    else
                        throw error::SyntaxError("no member separator");
                }
                else
                    firstMember = false;

                String memberName;
                if (parseString(is, memberName))
                {
                    skipCommentsAndWS(is);

                    cx = Traits::to_char_type(is.peek());
                    if (Traits::eq(cx, ':'))
                        is.ignore(1);
                    else
                        throw error::SyntaxError("no member value separator");

                    Value memberValue;
                    if (parse(is, memberValue))
                        jval[memberName].swap(memberValue);
                    else
                        throw error::SyntaxError("no member value");
                }
                else
                    throw error::SyntaxError("no member name");
            }
        }

        // array
        else if (Traits::eq(cx, '['))
        {
            is.ignore(1); // ignore '['
            Value(Value::TYPE_ARRAY).swap(jval);
            bool firstElement = true;

            while (is)
            {
                skipCommentsAndWS(is);

                cx = Traits::to_char_type(is.peek());
                if (Traits::eq(cx, ']'))
                {
                    is.ignore(1); // ignore ']'
                    break; // end of array
                }

                if (!firstElement)
                {
                    if (Traits::eq(cx, ','))
                    {
                        is.ignore(1); // ingore ','
                        skipCommentsAndWS(is);
                    }
                    else
                        throw error::SyntaxError("no element separator");
                }
                else
                    firstElement = false;

                Value elem;
                if (parse(is, elem))
                    jval.append(elem);
                else
                    throw error::SyntaxError("no element value");
            }
        }

        // number: integer or double
        else if (misc::is_digit(cx) || Traits::eq(cx, '+') || Traits::eq(cx, '-'))
        {
            Int64 ival = 0;
            if (is >> ival) // assume integer first
            {
                cx = Traits::to_char_type(is.peek()); // check for float-point
                if (Traits::eq(cx, '.') || Traits::eq(cx, 'e') || Traits::eq(cx, 'E'))
                {
                    double fval = 0.0;
                    if (is >> fval) // floating-point
                        Value(double(ival) + fval).swap(jval);
                    else
                        throw error::SyntaxError("cannot parse floating-point value");
                }
                else // integer
                    Value(ival).swap(jval);
            }
            else
                throw error::SyntaxError("cannot parse integer value");
        }

        // double-quoted or single-quoted string
        else if (Traits::eq(cx, '\"') || (HIVE_JSON_SINGLE_QUOTED_STRING && Traits::eq(cx, '\'')))
        {
            String val;
            if (parseQuotedString(is, val))
                Value(val).swap(jval);
            else
                throw error::SyntaxError("cannot parse string");
        }

        else if (Traits::eq(cx, 't') && match(is, "true"))
        {
            Value(true).swap(jval);
        }

        else if (Traits::eq(cx, 'f') && match(is, "false"))
        {
            Value(false).swap(jval);
        }

        else if (Traits::eq(cx, 'n') && match(is, "null"))
        {
            Value().swap(jval);
        }

        // extension: simple strings [0-9A-Za-z_] without quotes
        else if (HIVE_JSON_SIMPLE_STRING)
        {
            String val;
            if (parseSimpleString(is, val))
                Value(val).swap(jval);
            else
                throw error::SyntaxError("cannot parse simple string");
        }

        else
        {
            throw error::SyntaxError("no valid JSON value");
        }

        return is;
    }

public:

    /// @brief Skip comments and whitespaces.
    /**
    This method ignores all comments and whitespaces.

    @param[in,out] is The input stream.
    @return `false` if end of stream.
    @throw error::SyntaxError in case of invalid comment style
    */
    static bool skipCommentsAndWS(IStream &is)
    {
        // the maximum stream size
        const std::streamsize UP_TO_END = std::numeric_limits<std::streamsize>::max();

        while (is)
        {
            Traits::int_type meta = (is >> std::ws).peek();
            if (Traits::eq_int_type(meta, Traits::eof()))
                return false; // end of stream

            Traits::char_type cx = Traits::to_char_type(meta);

            // '#' comment
            if (Traits::eq(cx, '#'))
            {
                // ignore all the line
                is.ignore(UP_TO_END, '\n');
            }

            // C/C++ comments
            else if (Traits::eq(cx, '/'))
            {
                is.ignore(1); // ignore first '/'

                meta = is.peek();
                if (Traits::eq_int_type(meta, Traits::eof()))
                    return false; // end of stream

                cx = Traits::to_char_type(meta);
                if (Traits::eq(cx, '/')) // C++ style: one line
                {
                    // ignore all the line
                    is.ignore(UP_TO_END, '\n');
                }
                else if (Traits::eq(cx, '*')) // C style: /* ... */
                {
                    is.ignore(1); // ignore '*'
                    while (is) // search for "*/"
                    {
                        // skip all until '*'
                        is.ignore(UP_TO_END, '*');

                        meta = is.peek();
                        if (Traits::eq_int_type(meta, Traits::eof()))
                            return false; // end of stream

                        cx = Traits::to_char_type(meta);
                        if (Traits::eq(cx, '/'))
                        {
                            is.ignore(1); // ignore '/'
                            break;
                        }
                    }
                }
                else
                    throw error::SyntaxError("unknown comment style");
            }

            else
                return true; // OK
        }

        return false;
    }


    /// @brief Parse quoted or simple string from an input stream.
    /**
    @param[in,out] is The input stream.
    @param[out] str The parsed string.
    @return `true` if string successfully parsed.
    @throw error::SyntaxError in case of parsing error.
    */
    static bool parseString(IStream &is, String &str)
    {
        // remember the "quote" character
        const Traits::int_type QUOTE = is.peek();

        switch (QUOTE)
        {
            case '\"':  return parseQuotedString(is, str);
#if HIVE_JSON_SINGLE_QUOTED_STRING
            case '\'':  return parseQuotedString(is, str);
#endif // HIVE_JSON_SINGLE_QUOTED_STRING
#if HIVE_JSON_SIMPLE_STRING
            default:    return parseSimpleString(is, str);
#endif // HIVE_JSON_SIMPLE_STRING
        }

        return false;
    }


    /// @brief Parse quoted string from an input stream.
    /**
    @param[in,out] is The input stream.
    @param[out] str The parsed string.
    @return `true` if string successfully parsed.
    @throw error::SyntaxError in case of parsing error.
    */
    static bool parseQuotedString(IStream &is, String &str)
    {
        // remember the "quote" character
        const Traits::char_type QUOTE = Traits::to_char_type(is.get());

        OStringStream oss;
        while (is)
        {
            Traits::int_type meta = is.get();
            if (Traits::eq_int_type(meta, Traits::eof()))
                return false; // end of stream

            Traits::char_type ch = Traits::to_char_type(meta);
            if (ch == QUOTE)
            {
                str = oss.str();
                return true; // OK
            }

            else if (ch == '\\') // escape
            {
                meta = is.get();
                if (Traits::eq_int_type(meta, Traits::eof()))
                    return false; // end of stream

                ch = Traits::to_char_type(meta);
                switch (ch)
                {
                    case '"':  oss.put('"');  break;
                    case '\'': oss.put('\''); break;
                    case '/':  oss.put('/');  break;
                    case '\\': oss.put('\\'); break;
                    case 'b':  oss.put('\b'); break;
                    case 'f':  oss.put('\f'); break;
                    case 'n':  oss.put('\n'); break;
                    case 'r':  oss.put('\r'); break;
                    case 't':  oss.put('\t'); break;

                    case 'u':
                    {
                        const int a = misc::hex2int(is.get());
                        const int b = misc::hex2int(is.get());
                        const int c = misc::hex2int(is.get());
                        const int d = misc::hex2int(is.get());

                        if (a<0 || b<0 || c<0 || d<0)
                            throw error::SyntaxError("invalid UNICODE codepoint");

                        const int code = (a<<12) | (b<<8) | (c<<4) | d;

                        if (256 <= code)
                            throw error::SyntaxError("UNICODE not fully implemented yet");
                        oss.put(code); // TODO: convert to UTF-8 charset!!!
                    } break;

                    default:
                        throw error::SyntaxError("bad escape sequence in a string");
                }
            }

            else
                oss.put(ch); // just save
        }

        return false;
    }


    /// @brief Parse simple string from an input stream.
    /**
    A simple string consists of characters from the [0-9A-Za-z_] set.
    @param[in,out] is The input stream.
    @param[out] str The parsed string.
    @return `true` if string successfully parsed.
    @throw error::SyntaxError in case of parsing error.
    */
    static bool parseSimpleString(IStream &is, String &str)
    {
        OStringStream oss;
        while (is)
        {
            Traits::int_type meta = is.peek();
            if (Traits::eq_int_type(meta, Traits::eof()))
                return false; // end of stream

            if (Formatter::isSimple(meta))
            {
                oss.put(Traits::to_char_type(meta)); // just save
                is.ignore(1);
            }
            else
            {
                str = oss.str();
                return !str.empty(); // OK
            }
        }

        return false;
    }


    /// @brief Match the input with the pattern.
    /**
    @param[in,out] is The input stream.
    @param[in] pattern The pattern to match.
    @return `true` if matched, `false` if not match or end of stream.
    */
    static bool match(IStream &is, const char *pattern)
    {
        for (size_t i = 0; pattern[i]; ++i)
        {
            const Traits::int_type meta = is.peek();
            if (Traits::eq_int_type(meta, Traits::eof()))
                return false; // end of stream

            const Traits::char_type ch = Traits::to_char_type(meta);
            if (!Traits::eq(ch, pattern[i]))
            {
                for (; 0 < i; --i) // rollback
                    is.putback(pattern[i-1]);

                return false; // doesn't match
            }
            else
                is.ignore(1);
        }

        return true; // match
    }
};


/// @brief Parse JSON value from an input stream.
/** @relates Value
@param[in,out] is The input stream.
@param[in,out] jval The JSON value.
@return The input stream.
@throw error::SyntaxError in case of parsing error.
*/
inline IStream& operator>>(IStream &is, Value &jval)
{
    return Parser::parse(is, jval);
}


/// @brief Parse JSON value from string.
/** @relates Value
@param[in] str The JSON string.
@return The parsed JSON value.
@throw error::SyntaxError in case of parsing error.
*/
inline Value fromStr(String const& str)
{
    Value jval;
    IStringStream iss(str);
    Parser::parse(iss, jval);
    if (!(iss >> std::ws).eof()) // check is 'str' if fully parsed
        throw error::SyntaxError("partially parsed");
    return jval;
}

    } // json namespace
} // hive namespace

#endif // __HIVE_JSON_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_json JSON module

This page is under construction!
================================

[RFC-4627](http://tools.ietf.org/html/rfc4627)

This module provides support for JSON data.

hive::json namespace

hive::json::Value is the key class.
*/
