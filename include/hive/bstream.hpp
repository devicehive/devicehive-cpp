/** @file
@brief The binary serialization.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_hive_bstream
*/
#ifndef __HIVE_BSTREAM_HPP_
#define __HIVE_BSTREAM_HPP_

#include "defs.hpp"

#if !defined(HIVE_PCH)
#   include <istream>
#   include <ostream>
#endif // HIVE_PCH


namespace hive
{
    namespace io
    {

/// @brief The binary output stream.
/**
Writes formatted data to the standart output stream.

Uses host byte order format!!!
*/
// TODO: check errors!
class BinaryOStream
{
public:

    /// @brief The main constructor.
    /**
    @param[in] stream The low-level output stream.
    */
    explicit BinaryOStream(OStream & stream)
        : m_stream(stream)
    {}


    /// @brief Get the corresponding stream.
    OStream& getStream()
    {
        return m_stream;
    }

/// @name Fixed-size integers
/// @{
public:

    /// @brief Write unsigned 8-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putUInt8(UInt8 val)
    {
        putIntX(val);
    }


    /// @brief Write signed 8-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putInt8(Int8 val)
    {
        putIntX(val);
    }


    /// @brief Write unsigned 16-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putUInt16(UInt16 val)
    {
        putIntX(val);
    }


    /// @brief Write signed 16-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putInt16(Int16 val)
    {
        putIntX(val);
    }


    /// @brief Write unsigned 32-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putUInt32(UInt32 val)
    {
        putIntX(val);
    }


    /// @brief Write signed 32-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putInt32(Int32 val)
    {
        putIntX(val);
    }


    /// @brief Write unsigned 64-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putUInt64(UInt64 val)
    {
        putIntX(val);
    }


    /// @brief Write signed 64-bits integer.
    /**
    @param[in] val The value to write.
    */
    void putInt64(Int64 val)
    {
        putIntX(val);
    }
/// @}

/// @name Variable-size integers
/// @{
public:

    /// @brief Write unsigned 32-bits integer in variable size format.
    /**
    @param[in] val The value to write.
    */
    void putUInt32V(UInt32 val)
    {
        putIntXV(val);
    }


    /// @brief Write unsigned 64-bits integer in variable size format.
    /**
    @param[in] val The value to write.
    */
    void putUInt64V(UInt64 val)
    {
        putIntXV(val);
    }


    /// @brief Write signed 32-bits integer in variable size format (zig-zag mode).
    /**
    @param[in] val The value to write.
    */
    void putInt32VZ(Int32 val)
    {
        putUInt32V((val<<1) ^ (val>>31));
    }


    /// @brief Write signed 64-bits integer in variable size format (zig-zag mode).
    /**
    @param[in] val The value to write.
    */
    void putInt64VZ(Int64 val)
    {
        putUInt64V((val<<1) ^ (val>>63));
    }

/// @}

/// @name String and custom data buffer
/// @{
public:

    /// @brief Write the custom string.
    /**
    Writes length and raw data. No encoding changes.

    @param[in] val The value to write.
    */
    void putString(String const& val)
    {
        putUInt32V(UInt32(val.size()));
        m_stream.write(val.data(), val.size());
    }


    /// @brief Write custom data buffer.
    /**
    @param[in] buf The buffer to write.
    @param[in] len The buffer length in bytes.
    */
    void putBuffer(const void *buf, size_t len)
    {
        m_stream.write(static_cast<const char*>(buf), len);
    }
/// @}

private:

    /// @brief Write custom integer.
    /**
    @param[in] val The value to write.
    */
    template<typename IntX>
    void putIntX(IntX val)
    {
        /// @brief The raw converter.
        union Val2Raw
        {
            char raw[sizeof(IntX)]; ///< @brief The RAW bytes.
            IntX val;               ///< @brief The integer.
        };

        Val2Raw buf;
        buf.val = val;

        //static_assert(sizeof(val) == sizeof(buf))
        m_stream.write(buf.raw, sizeof(Val2Raw));
    }


    /// @brief Write custom integer in variable size format.
    /**
    @param[in] val The value to write.
    */
    template<typename UIntX>
    void putIntXV(UIntX val)
    {
        // go through all bytes, LSB-first
        for (size_t b = 0; b < sizeof(UIntX); ++b)
        {
            // data (7 bits)
            const int d = int(val)&0x7F;
            val >>= 7;

            // 'continue' flag
            const int f = (0 != val);

            m_stream.put((f<<7) | d);
            if (!f)
                break;
        }
    }

private:
    OStream & m_stream; ///< @brief The low-level output stream.
};


/// @brief The binary input stream.
/**
Reads formatted data from the standart input stream.

Uses host byte order format!!!
*/
// TODO: check errors!!!
class BinaryIStream
{
public:

    /// @brief The main constructor.
    /**
    @param[in] stream The low-level input stream.
    */
    explicit BinaryIStream(IStream & stream)
        : m_stream(stream)
    {}


    /// @brief Get the corresponding stream.
    IStream& getStream()
    {
        return m_stream;
    }

/// @name Fixed-size integers
/// @{
public:

    /// @brief Read unsigned 8-bits integer.
    /**
    @return The read value.
    */
    UInt8 getUInt8()
    {
        return getIntX<UInt8>();
    }


    /// @brief Read signed 8-bits integer.
    /**
    @return The read value.
    */
    Int8 getInt8()
    {
        return getIntX<Int8>();
    }


    /// @brief Read unsigned 16-bits integer.
    /**
    @return The read value.
    */
    UInt16 getUInt16()
    {
        return getIntX<UInt16>();
    }


    /// @brief Read signed 16-bits integer.
    /**
    @return The read value.
    */
    Int16 getInt16()
    {
        return getIntX<Int16>();
    }


    /// @brief Read unsigned 32-bits integer.
    /**
    @return The read value.
    */
    UInt32 getUInt32()
    {
        return getIntX<UInt32>();
    }


    /// @brief Read signed 32-bits integer.
    /**
    @return The read value.
    */
    Int32 getInt32()
    {
        return getIntX<Int32>();
    }


    /// @brief Read unsigned 64-bits integer.
    /**
    @return The read value.
    */
    UInt64 getUInt64()
    {
        return getIntX<UInt64>();
    }


    /// @brief Read signed 64-bits integer.
    /**
    @return The read value.
    */
    Int64 getInt64()
    {
        return getIntX<Int64>();
    }

/// @}

/// @name Variable-size integers
/// @{
public:

    /// @brief Read unsigned 32-bits integer in variable size format.
    /**
    @return The read value.
    */
    UInt32 getUInt32V()
    {
        return getIntXV<UInt32>();
    }


    /// @brief Read unsigned 64-bits integer in variable size format.
    /**
    @return The read value.
    */
    UInt64 getUInt64V()
    {
        return getIntXV<UInt64>();
    }


    /// @brief Read signed 32-bits integer in variable size format (zig-zag mode).
    /**
    @return The read value.
    */
    Int32 getInt32VZ()
    {
        const UInt32 val = getUInt32V();
        return (val>>1) ^ -Int32(val&1);
    }


    /// @brief Read signed 64-bits integer in variable size format (zig-zag mode).
    /**
    @return The read value.
    */
    Int64 getInt64VZ()
    {
        const UInt64 val = getUInt64V();
        return (val>>1) ^ -Int64(val&1);
    }

/// @}

/// @name String and custom data buffer
/// @{
public:

    /// @brief Read the custom string.
    /**
    @return The read value.
    */
    String getString()
    {
        size_t len = size_t(getUInt32V());
        String res; res.reserve(len);

        char buf[64]; // "read" buffer
        while (0 < len)
        {
            const size_t n = (len < sizeof(buf))
                ? len : sizeof(buf); // minimum

            m_stream.read(buf, n);
            res.append(buf, buf+n);

            len -= n;
        }

        return res;
    }


    /// @brief Read custom data buffer.
    /**
    @param[in] buf The buffer read to.
    @param[in] len The buffer length in bytes.
    */
    void getBuffer(void *buf, size_t len)
    {
        m_stream.read(static_cast<char*>(buf), len);
    }

/// @}

private:

    /// @brief Read custom integer.
    /**
    @return The read value.
    */
    template<typename IntX>
    IntX getIntX()
    {
        /// @brief The raw converter.
        union Val2Raw
        {
            char raw[sizeof(IntX)]; ///< @brief The RAW bytes.
            IntX val;               ///< @brief The integer.
        };

        Val2Raw buf;

        // static_assert(sizeof(val) == sizeof(buf))
        buf.val = 0; // default value
        m_stream.read(buf.raw, sizeof(Val2Raw));
        return buf.val;
    }


    /// @brief Read custom integer in variable size format.
    /**
    @return The read value.
    */
    template<typename UIntX>
    UIntX getIntXV()
    {
        UIntX val = 0;

        // go through all bytes, LSB-first
        for (size_t i = 0; i < sizeof(UIntX); ++i)
        {
            // 'continue' flag and data (7 bits)
            int f_d = m_stream.get();

            // data (7 bits)
            val |= UIntX(f_d&0x7F) << (i*7);

            // 'continue'
            if (!(f_d&0x80))
                break;
        }

        return val;
    }

private:
    IStream & m_stream; ///< @brief The low-level input stream.
};

    } // io namespace
} // hive namespace

#endif // __HIVE_BSTREAM_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_bstream Binary streams

This page is under construction!
================================

Binary formats. hive::io::BinaryOStream and hive::io::BinaryIStream should
be used with std::stringstream or boost::asio::streambuf.

Variable size format.

Zig-Zag format for signed integers.

Examples.
*/
