/** @file
@brief The common definitions.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_defs
*/
#ifndef __HIVE_DEFS_HPP_
#define __HIVE_DEFS_HPP_

#if !defined(HIVE_PCH)
#   include <boost/noncopyable.hpp>
#   include <boost/cstdint.hpp>
#   include <iosfwd>
#   include <string>
#endif // HIVE_PCH


/// @brief The main namespace.
/**
This is the root namespace for all basic tools.

@see @ref page_hive
*/
namespace hive
{

/// @name Fixed-size integers
/// @{
typedef boost::uint8_t  UInt8;  ///< @brief The 8-bits unsigned integer.
typedef boost::uint16_t UInt16; ///< @brief The 16-bits unsigned integer.
typedef boost::uint32_t UInt32; ///< @brief The 32-bits unsigned integer.
typedef boost::uint64_t UInt64; ///< @brief The 64-bits unsigned integer.

typedef boost::int8_t  Int8;  ///< @brief The 8-bits signed integer.
typedef boost::int16_t Int16; ///< @brief The 16-bits signed integer.
typedef boost::int32_t Int32; ///< @brief The 32-bits signed integer.
typedef boost::int64_t Int64; ///< @brief The 64-bits signed integer.
/// @}


/// @name String and streams
/// @{
typedef std::ostringstream OStringStream; ///< @brief The output string stream.
typedef std::istringstream IStringStream; ///< @brief The input string stream.
typedef std::stringstream   StringStream; ///< @brief The input/output string stream.
typedef std::string         String;       ///< @brief The string.
typedef std::ostream             OStream; ///< @brief The basic output stream.
typedef std::istream             IStream; ///< @brief The basic input stream.
/// @}


/// @brief The non-copyable interface.
/**
This class should be used as private base class to emphasise
that object cannot be copied. It is especially useful for classes
which holds some h/w resources and therefore cannot be copied.

~~~{.cpp}
class MyDevice:
    private hive::NonCopyable
{
public:
    int foo;
};

void f()
{
    MyDevice a;
    MyDevice b(a); // error!
    b = a;         // error!
}
~~~

@note Anyway it is possible to explicit provide copy-constructor
      and/or assignment operator for derived classes.
*/
typedef boost::noncopyable NonCopyable;


    /// @brief The miscellaneous tools.
    /**
    This namespace contains various auxiliary tools.
    */
    namespace misc
    {}


// HIVE_HAS_RVALUE_REFS
#if !defined(HIVE_HAS_RVALUE_REFS) // auto detection
#   if defined(_MSC_VER) && 1600 <= _MSC_VER
#       define HIVE_HAS_RVALUE_REFS
#   endif // _MSC_VER
// TODO: detect HIVE_HAS_RVALUE_REFS on gcc
#endif // !defined(HIVE_HAS_RVALUE_REFS)
#if defined(HIVE_DOXY_MODE)
#undef HIVE_HAS_RVALUE_REFS

/// @hideinitializer @brief Enable rvalue references.
/**
If your compiler is modern enough, you can define #HIVE_HAS_RVALUE_REFS macro
to enable move contructors and move assignment operators. This may increase
the execution speed for some data type by eliminating copying.
*/
#define HIVE_HAS_RVALUE_REFS
#endif // defined(HIVE_DOXY_MODE)


// HIVE_UNUSED
#if !defined(HIVE_UNUSED)
#   define HIVE_UNUSED(x) (void)x
#endif // !defined(HIVE_UNUSED)
#if defined(HIVE_DOXY_MODE)
#undef HIVE_UNUSED

/// @hideinitializer @brief Enable rvalue references.
/**
You can use special macro #HIVE_UNUSED to mark any variable as "used"
just to prevent "unreferenced formal parameter" warnings.
*/
#define HIVE_UNUSED(x)
#endif // defined(HIVE_DOXY_MODE)

} // hive namespace

#endif // __HIVE_DEFS_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_defs Common definitions

The fixed-size integers
-----------------------

The fixed-size integer is very useful for binary protocols.
See also @ref page_hive_swab page for various byte ordering functions.

The following typedefs are avaialble for signed integers:
- hive::Int8
- hive::Int16
- hive::Int32
- hive::Int64

The following typedefs are available for unsigned integers:
- hive::UInt8
- hive::UInt16
- hive::UInt32
- hive::UInt64


String and streams
------------------

The main purpose of these typedes is possibility to use UNICODE strings
and streams in the future.

The hive::String is used as container for string data.

Also the following stream typedefs are available:
- hive::OStringStream
- hive::IStringStream
- hive::StringStream
- hive::OStream
- hive::IStream


Other typedefs
--------------

The hive::NonCopyable interface is used as "not permitted to copy" indicator.
*/
