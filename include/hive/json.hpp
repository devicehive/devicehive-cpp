/** @file
@brief The JSON classes.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

Some implementation is based on http://jsoncpp.sourceforge.net/ library by Baptiste Lepilleur.

See also RFC-4627 http://tools.ietf.org/html/rfc4627

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

namespace hive
{
    /// @brief The JSON module.
    /**
    This namespace contains classes and functions related to JSON data processing.
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
The JSON value may be one of the following:
    - NULL
    - boolean
    - integer
    - double
    - string
    - array
    - object

JSON conversions {#hive_json_value_conv}
========================================

Some of JSON values may be converted to the another. There is the conversion table:
    - to boolean:
        - from NULL is always converted as `false`
        - zero integers as `false`, non-zero as `true`
        - zero floating-point as `false`, non-zero as `true`
        - empty string as `false`
        - "false" string as `false`
        - "true" string as `true`
    - to integer:
        - from NULL as `0`
        - from boolean as `0` or `1`
        - empty string as `0`
        - from string parsed value
    - to double:
        - from NULL as `0.0`
        - from boolean as `0.0` or `1.0`
        - from integers
        - empty string as `0.0`
        - from string parsed value
    - to string:
        - from NULL as empty string
        - from boolean as `"true"` or `"false"`
        - from integers
        - from floating-point
    - to array:
        - from NULL as empty array
    - to object:
        - from NULL as empty object
*/
class Value
{
public:

    /// @brief The value type.
    enum Type
    {
        TYPE_NULL,    ///< @brief The NULL value.
        TYPE_BOOLEAN, ///< @brief The boolean value.
        TYPE_INTEGER, ///< @brief The integer value.
        TYPE_DOUBLE,  ///< @brief The floating-point value.
        TYPE_STRING,  ///< @brief The string value.
        TYPE_ARRAY,   ///< @brief The array value.
        TYPE_OBJECT   ///< @brief The object value.
    };

/// @name Constructors
/// @{
public:

    /// @brief The default constructor.
    /**
    This constructor may be used to make empty arrays or objects.

    @param[in] type The value type (NULL by default).
    */
    explicit Value(Type type = TYPE_NULL)
        : m_type(type)
    {
        m_val.u = 0;
    }


    /// @brief Construct the boolean value.
    /**
    @param[in] val The boolean value.
    */
    /*explicit*/ Value(bool val)
        : m_type(TYPE_BOOLEAN)
    {
        m_val.u = val?1:0;
    }


    /// @brief Construct the integer value (8 bits).
    /**
    @param[in] val The signed integer value.
    */
    /*explicit*/ Value(Int8 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the integer value (8 bits).
    /**
    @param[in] val The unsigned integer value.
    */
    /*explicit*/ Value(UInt8 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.u = val;
    }


    /// @brief Construct the integer value (16 bits).
    /**
    @param[in] val The signed integer value.
    */
    /*explicit*/ Value(Int16 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the integer value (16 bits).
    /**
    @param[in] val The unsigned integer value.
    */
    /*explicit*/ Value(UInt16 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.u = val;
    }


    /// @brief Construct the integer value (32 bits).
    /**
    @param[in] val The signed integer value.
    */
    /*explicit*/ Value(Int32 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the integer value (32 bits).
    /**
    @param[in] val The unsigned integer value.
    */
    /*explicit*/ Value(UInt32 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.u = val;
    }


    /// @brief Construct the integer value (64 bits).
    /**
    @param[in] val The signed integer value.
    */
    /*explicit*/ Value(Int64 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.i = val;
    }


    /// @brief Construct the integer value (64 bits).
    /**
    @param[in] val The unsigned integer value.
    */
    /*explicit*/ Value(UInt64 val)
        : m_type(TYPE_INTEGER)
    {
        m_val.u = val;
    }


    /// @brief Construct the floating-point value.
    /**
    @param[in] val The floating-point value.
    */
    /*explicit*/ Value(double val)
        : m_type(TYPE_DOUBLE)
    {
        m_val.f = val;
    }


    /// @brief Construct the floating-point value.
    /**
    @param[in] val The floating-point value.
    */
    /*explicit*/ Value(float val)
        : m_type(TYPE_DOUBLE)
    {
        m_val.f = val;
    }


    /// @brief Construct the string value.
    /**
    @param[in] val The string value.
    */
    /*explicit*/ Value(const char* val)
        : m_type(TYPE_STRING)
        , m_str(val)
    {
        m_val.u = 0; // avoid garbage
    }


    /// @brief Constring the string value.
    /**
    @param[in] val The string value.
    */
    /*explicit*/ Value(String const& val)
        : m_type(TYPE_STRING)
        , m_str(val)
    {
        m_val.u = 0; // avoid garbage
    }
/// @}


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
        Value tmp(other);
        swap(tmp);
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

public:

    /// @brief Get the NULL value.
    /**
    This static value is used to indicate the NULL value references.

    @return The NULL value static reference.
    */
    static Value const& null()
    {
        static Value N;
        return N;
    }


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


    /// @brief Is the value NULL?
    /**
    @return `true` if the value is NULL.
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


    /// @brief Is the value boolean?
    /**
    @return `true` if the value is boolean.
    */
    bool isBool() const
    {
        return (TYPE_BOOLEAN == m_type);
    }


    /// @brief Is the value integer?
    /**
    @return `true` if the value is integer.
    */
    bool isInteger() const
    {
        return (TYPE_INTEGER == m_type);
    }


    /// @brief Is the value floating-point?
    /**
    @return `true` if the value if floating-point.
    */
    bool isDouble() const
    {
        return (TYPE_DOUBLE == m_type);
    }


    /// @brief Is the value string?
    /**
    @return `true` if the value is string.
    */
    bool isString() const
    {
        return (TYPE_STRING == m_type);
    }


    /// @brief Is the value array or NULL?
    /**
    @return `true` if the value is array or NULL.
    */
    bool isArray() const
    {
        return (TYPE_NULL == m_type) || (TYPE_ARRAY == m_type);
    }


    /// @brief Is the value object or NULL?
    /**
    @return `true` if the value is object or NULL.
    */
    bool isObject() const
    {
        return (TYPE_NULL == m_type) || (TYPE_OBJECT == m_type);
    }


    /// @brief Is the value convertible to the other type?
    /**
    See @ref hive_json_value_conv for all possible conversions.

    @param[in] type The type convert to.
    @return `true` if the value is convertible.
    @see asBool()
    @see asInt()
    @see asUInt()
    @see asDouble()
    @see asString()
    */
    bool isConvertibleTo(Type type) const
    {
        switch (m_type)
        {
            case TYPE_NULL: // NULL -> ...
                return true;

            case TYPE_BOOLEAN: // boolean -> ...
                return (TYPE_NULL == type && !m_val.u)
                    || (TYPE_BOOLEAN == type)
                    || (TYPE_INTEGER == type)
                    || (TYPE_DOUBLE == type)
                    || (TYPE_STRING == type);

            case TYPE_INTEGER: // integer -> ...
                return (TYPE_NULL == type &&  !m_val.u)
                    || (TYPE_BOOLEAN == type)
                    || (TYPE_INTEGER == type)
                    || (TYPE_DOUBLE == type)
                    || (TYPE_STRING == type);

            case TYPE_DOUBLE: // double -> ...
                return (TYPE_NULL == type && m_val.f != 0.0)
                    || (TYPE_BOOLEAN == type)
                    || (TYPE_INTEGER == type)
                    || (TYPE_DOUBLE == type)
                    || (TYPE_STRING == type);

            case TYPE_STRING: // string -> ...
                if (TYPE_BOOLEAN == type) // -> boolean
                {
                    return m_str.empty()
                        || m_str=="false"
                        || m_str=="true";
                }
                else if (TYPE_INTEGER == type) // -> integer
                {
                    const size_t N = m_str.size();
                    if (0 < N)
                    {
                        size_t i = 0;

                        // ignore leading '+' or '-'
                        if ('+'==m_str[i] || '-'==m_str[i])
                        {
                            if (N == ++i)
                                return false; // one sign isn't allowed
                        }

                        for (; i < N; ++i)
                            if (!misc::is_digit(m_str[i]))
                                return false;
                    }

                    return true;
                }
                else if (TYPE_DOUBLE == type) // -> double
                {
                    const size_t N = m_str.size();
                    if (0 < N)
                    {
                        // TODO: check m_str is double
                        IStringStream iss(m_str);
                        double val = 0.0;
                        if (!(iss >> val))
                            return false;
                    }

                    return true;
                }
                else
                    return (TYPE_NULL == type && m_str.empty())
                        || (TYPE_STRING == type);

            case TYPE_ARRAY:
                return (TYPE_NULL == type && m_arr.empty())
                    || (TYPE_ARRAY == type);

            case TYPE_OBJECT:
                return (TYPE_NULL == type && m_obj.empty())
                    || (TYPE_OBJECT == type);
        }

        return false; // no conversion
    }
/// @}

public:

    /// @brief Convert value to the boolean.
    /**
    @return The boolean value.
    @throw error::CastError if cannot convert.
    @see @ref hive_json_value_conv
    */
    bool asBool() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return false; // NULL as false

            case TYPE_BOOLEAN:
            case TYPE_INTEGER: // true as non-zero
                return (0 != m_val.u);

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


    /// @brief Convert value to the signed integer.
    /**
    @return The signed integer value.
    @throw error::CastError if cannot convert.
    @see @ref hive_json_value_conv
    */
    Int64 asInt() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return 0; // NULL as zero

            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
                return m_val.i;

            case TYPE_DOUBLE:
                return Int64(m_val.f);

            case TYPE_STRING:
            {
                if (!m_str.empty())
                {
                    if (isConvertibleTo(TYPE_INTEGER))
                    {
                        IStringStream iss(m_str);
                        Int64 val = 0;
                        if (iss >> val)
                            return val;
                    }
                }
                else
                    return 0;
            } break;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to integer");
    }


    /// @brief Convert value to the unsigned integer.
    /**
    @return The unsigned integer value.
    @throw error::CastError if cannot convert.
    @see @ref hive_json_value_conv
    */
    UInt64 asUInt() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return 0; // NULL as zero

            case TYPE_BOOLEAN:
            case TYPE_INTEGER:
                return m_val.u;

            case TYPE_DOUBLE:
                return UInt64(m_val.f);

            case TYPE_STRING:
            {
                if (!m_str.empty())
                {
                    if (isConvertibleTo(TYPE_INTEGER))
                    {
                        IStringStream iss(m_str);
                        Int64 val = 0;
                        if (iss >> val)
                            return val;
                    }
                }
                else
                    return 0;
            } break;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to integer");
    }


    /// @brief Convert value to the floating-point.
    /**
    @return The floating-point value.
    @throw error::CastError if cannot convert.
    @see @ref hive_json_value_conv
    */
    double asDouble() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return 0.0;

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
                    if (iss >> val)
                        return val;
                }
                else
                    return 0.0;
            } break;

            case TYPE_ARRAY:
            case TYPE_OBJECT:
                break; // no conversion
        }

        // TODO: detail information about value type?
        throw error::CastError("cannot convert to floating-point");
    }


    /// @brief Convert value to the string.
    /**
    @return The floating-point value.
    @throw error::CastError if cannot convert.
    @see @ref hive_json_value_conv
    */
    String asString() const
    {
        switch (m_type)
        {
            case TYPE_NULL:
                return String(); // NULL as empty string

            case TYPE_BOOLEAN:
                return m_val.u ? "true" : "false";

            case TYPE_INTEGER:
            {
                OStringStream oss;
                oss << m_val.i;
                return oss.str();
            }

            case TYPE_DOUBLE:
            {
                OStringStream oss;
                oss << m_val.f; // default format
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
                return m_val.u == other.m_val.u;

            case TYPE_DOUBLE:
                return m_val.f == other.m_val.f;

            case TYPE_STRING:
                return m_str == other.m_str;

            case TYPE_ARRAY:
                return m_arr.size() == other.m_arr.size()
                    && std::equal(m_arr.begin(), m_arr.end(),
                            other.m_arr.begin());

            case TYPE_OBJECT:
                return m_obj.size() == other.m_obj.size()
                    && std::equal(m_obj.begin(), m_obj.end(),
                            other.m_obj.begin());
        }

        return false;
    }

public: // array & object

    /// @brief Get the number of elements in array or object.
    /**
    For all other types returns zero.

    @return The number of array elements or the number of members.
    */
    size_t size() const
    {
        switch (m_type)
        {
            case TYPE_ARRAY:
                return m_arr.size();

            case TYPE_OBJECT:
                return m_obj.size();

            default:
                break;
        }

        return 0; // otherwise
    }


    /// @brief Is value empty?
    /**
    @return `true` if array is empty, object is empty, or value is NULL.
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
                break;
        }

        return false; // otherwise
    }


    /// @brief Remove all object members or array elements.
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

/// @name Array
/// @{
public:

    /// @brief Resize the array.
    /**
    This method changes value type to TYPE_ARRAY.

    New array elements initialized to @a def values, NULL by default.

    @param[in] size The new array size.
    @param[in] def The default value to fill array.
    */
    void resize(size_t size, Value const& def = Value())
    {
        assert(isArray() && "not an array");
        if (m_type != TYPE_ARRAY)
            m_type = TYPE_ARRAY;
        m_arr.resize(size, def);
    }


    /// @brief Get an array element.
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


    /// @brief Get an array element (read-only).
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


    /// @brief Append value to array at the end.
    /**
    This method changes value type to TYPE_ARRAY.

    @param[in] jval The JSON value to append.
    */
    void append(Value const& jval)
    {
        assert(isArray() && "not an array");
        if (m_type != TYPE_ARRAY)
            m_type = TYPE_ARRAY;
        m_arr.push_back(jval);
    }


    /// @brief The array elements iterator.
    /**
    This type is used to iterate all elements in array JSON value.
    */
    typedef std::vector<Value>::const_iterator ElementIterator;


    /// @brief Get the begin of array.
    /**
    @return The begin of array.
    */
    ElementIterator elementsBegin() const
    {
        assert(isArray() && "not an array");
        return m_arr.begin();
    }


    /// @brief Get the end of array.
    /**
    @return The end of array.
    */
    ElementIterator elementsEnd() const
    {
        assert(isArray() && "not an array");
        return m_arr.end();
    }
/// @}


/// @name Object
/// @{
public:

    /// @brief Get the member by name.
    /**
    @param[in] name The member name.
    @param[in] def The default value if no member with such name exists.
    @return The member value or @a def if no member with such name exists.
    */
    Value const& get(String const& name, Value const& def) const
    {
        assert(isObject() && "not an object");

        Object::const_iterator m = m_obj.find(name);
        return (m == m_obj.end()) ? def : m->second;
    }


    /// @brief Get the member by name or create new one.
    /**
    This method changes value type to TYPE_OBJECT.

    NULL member value created if no member with such name exists.

    @param[in] name The member name.
    @return The member value.
    */
    Value& get(String const& name)
    {
        assert(isObject() && "not an object");

        Object::iterator m = m_obj.find(name);
        if (m == m_obj.end())
        {
            if (TYPE_NULL == m_type)
                m_type = TYPE_OBJECT;
            m = m_obj.insert(m, std::make_pair(name, Value()));
        }

        return m->second;
    }


    /// @brief Get the member by name.
    /**
    @param[in] name The member name.
    @return The member value or null().
    */
    Value const& operator[](String const& name) const
    {
        return get(name, null());
    }


    /// @brief Get the member by name or create new.
    /**
    NULL member value created if no member with such name exists.

    @param[in] name The member name.
    @return The member value.
    */
    Value& operator[](String const& name)
    {
        return get(name);
    }


    /// @brief Is member value exists?
    /**
    @param[in] name The member name.
    @return `true` if member with such name exists.
    */
    bool hasMemeber(String const& name) const
    {
        assert(isObject() && "not an object");

        Object::const_iterator m = m_obj.find(name);
        return (m != m_obj.end());
    }


    /// @brief Remove member.
    /**
    @param[in] name The member name to remove.
    */
    void removeMember(String const& name)
    {
        assert(isObject() && "not an object");
        m_obj.erase(name);
    }


    /// @brief The members iterator.
    /**
    This type is used to iterate all member names on object JSON value.
    */
    typedef std::map<String,Value>::const_iterator MemberIterator;


    /// @brief Get the begin of object's members.
    /**
    @return The begin of object's members.
    */
    MemberIterator membersBegin() const
    {
        assert(isObject() && "not an object");
        return m_obj.begin();
    }


    /// @brief Get the end of object's members.
    /**
    @return The end of object's members.
    */
    MemberIterator membersEnd() const
    {
        assert(isObject() && "not an object");
        return m_obj.end();
    }
/// @}

private:
    Type m_type; ///< @brief The value type.

    /// @brief The %POD data holder type.
    union POD
    {
        double f; ///< @brief The floating-point value.
        UInt64 u; ///< @brief The unsigned integer value.
         Int64 i; ///< @brief The signed integer value.
    } m_val; ///< @brief The %POD data holder.

    typedef std::vector<Value> Array; ///< @brief The array type.
    typedef std::map<String,Value> Object; ///< @brief The object type.

    // TODO: move these to union as pointers?
    String m_str; ///< @brief The string value.
    Array  m_arr; ///< @brief The array value.
    Object m_obj; ///< @brief The object value.
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
    return !(a == b);
}



/// @brief The basic JSON formatter.
/**
description is under construction.

Writes JSON value in compact format without spaces and new lines.
*/
class Formatter
{
public:

    /// @brief Write the JSON value to the output stream.
    /**
    In "human friendly" format there also the line feeds and indents in the output.
    Otherwise no any indents or spaces are used.

    @param[in,out] os The output stream.
    @param[in] jval The JSON value.
    @param[in] humanFriendly The human fiendly format flag.
    @param[in] indent The first line indent. Used for human friendly format.
    @return The output stream.
    */
    static OStream& write(OStream &os, Value const& jval, bool humanFriendly, size_t indent = 0)
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
                writeQuotedString(os, jval.asString());
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
                            indent+1);
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
                Value::MemberIterator b = jval.membersBegin();
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
                        writeQuotedString(os, name);
                        os.put(':');
                        if (humanFriendly)
                            os.put(' ');
                        write(os, val,
                            humanFriendly,
                            indent+1);
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

    /// @brief Write indent.
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


    /// @brief Write quoted string.
    /**
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
                    if (!(misc::is_char(ch) && !misc::is_ctl(ch)))
                    {
                        os.put('\\');
                        os.put('u');
                        os.put(misc::int2hex((ch>>12)&0x0f));
                        os.put(misc::int2hex((ch>>8)&0x0f));
                        os.put(misc::int2hex((ch>>4)&0x0f));
                        os.put(misc::int2hex((ch>>0)&0x0f));
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


/// @brief Write JSON value to the output stream.
/** @relates Value
@param[in,out] os The output stream.
@param[in] jval The JSON value.
@return The output stream.
*/
inline OStream& operator<<(OStream &os, const Value &jval)
{
    const bool humanFriendly = false; // true;
    return Formatter::write(os, jval, humanFriendly);
}


/// @brief Convert JSON value to string.
/**
@param[in] jval The JSON value.
@return The JSON string.
*/
inline String json2str(Value const& jval)
{
    OStringStream oss;
    Formatter::write(oss, jval, false);
    return oss.str();
}


/// @brief Convert JSON value to human friendly string.
/**
@param[in] jval The JSON value.
@return The JSON string.
*/
inline String json2hstr(Value const& jval)
{
    OStringStream oss;
    Formatter::write(oss, jval, true);
    return oss.str();
}


/// @brief The JSON parser.
/**
description is under construction.
*/
class Parser
{
    /// @brief The string traits type.
    typedef String::traits_type Traits;
public:

    /// @brief Parse the JSON value from the input stream.
    /**
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
                if (parseQuotedString(is, memberName))
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

            while (1)
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

        // string
        else if (Traits::eq(cx, '\"'))
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

        else
        {
            throw error::SyntaxError("no valid JSON value");
        }

        return is;
    }

public:

    /// @brief Skip whitespaces and comments
    /**
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
                    while (1) // search for "*/"
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


    /// @brief Parse quoted string from the input stream.
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
                        throw error::SyntaxError("unicode not implemented yet");
                        //unsigned int unicode;
                        //if ( !decodeUnicodeCodePoint( is, unicode ) )
                        //    return false;
                        //oss << codePointToUTF8(unicode);
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
                //for (; 0 < i; --i) // rollback
                //    is.putback(pattern[i-1]);

                return false; // doesn't match
            }
            else
                is.ignore(1);
        }

        return true; // match
    }
};


/// @brief Parse JSON value from the input stream.
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


/// @brief Convert string to JSON value.
/**
@param[in] str The JSON string.
@return The parsed JSON value.
@throw error::SyntaxError in case of parsing error.
*/
inline Value str2json(String const& str)
{
    Value jval;
    IStringStream iss(str);
    Parser::parse(iss, jval);
    (iss >> std::ws).peek();
    if (!iss.eof()) // check is 'str' if fully parsed
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

This module provides support for JSON data.

hive::json namespace

hive::json::Value is the key class.
*/
