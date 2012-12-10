/** @file
@brief The XBee API prototype (experimental).
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __DEVICEHIVE_XBEE_HPP_
#define __DEVICEHIVE_XBEE_HPP_

#include <hive/binary.hpp>
#include <hive/dump.hpp>
#include <hive/log.hpp>

#if !defined(HIVE_PCH)
#   include <boost/enable_shared_from_this.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <boost/bind.hpp>
#   include <vector>
#endif // HIVE_PCH


/// @brief The XBee %API mode prototype (experimental).
namespace xbee
{
    using namespace hive;


/// @brief The XBee frame.
/**
This class contains full formated frame:

|     field | size in bytes
|-----------|--------------
| signature | 1
|    length | 2
|   payload | N
|  checksum | 1

The empty frame doesn't contain any data.

There are a few payloads:
    - ATCommandRequest
    - ATCommandResponse
    - ZBTransmitRequest
    - ZBTransmitStatus
    - ZBReceivePacket
*/
class Frame:
    public bin::FrameContent
{
public:

    /// @brief Constants.
    enum Const
    {
        SIGNATURE = 0x7E, ///< @brief The signature byte.
        ESCAPE    = 0x7D, ///< @brief The escape byte.

        SIGNATURE_LEN = 1, ///< @brief The signature field length in bytes.
        LENGTH_LEN    = 2, ///< @brief The length field length in bytes.
        CHECKSUM_LEN  = 1, ///< @brief The checksum field length in bytes.

        /// @brief The total header length in bytes.
        HEADER_LEN = SIGNATURE_LEN + LENGTH_LEN,

        /// @brief The total footer length in bytes.
        FOOTER_LEN = CHECKSUM_LEN
    };

    /// @brief The frame types (API command identifiers).
    enum Type
    {
        ZB_TRANSMIT_REQUEST       = 0x10,
        ZB_TRANSMIT_STATUS        = 0x8B,
        ZB_RECEIVE_PACKET         = 0x90,
        ATCOMMAND_REQUEST         = 0x08,
        ATCOMMAND_RESPONSE        = 0x88,
        REMOTE_ATCOMMAND_REQUEST  = 0x17,
        REMOTE_ATCOMMAND_RESPONSE = 0x97
    };

public: // common payloads

    class Payload;
        class ATCommandRequest;
        class ATCommandResponse;
        class ZBTransmitRequest;
        class ZBTransmitStatus;
        class ZBReceivePacket;

protected:

    /// @brief The default constructor.
    /**
    Constructs the empty frame.
    */
    Frame()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Frame> SharedPtr;


    /// @brief Construct the frame from the payload.
    /**
    @param[in] payload The data payload.
    Should be byte container with `format()` method.
    @return The new frame instance.
    */
    template<typename PayloadT>
    static SharedPtr create(PayloadT const& payload)
    {
        OStringStream oss;
        io::BinaryOStream bs(oss);
        payload.format(bs);

        SharedPtr pthis(new Frame());
        pthis->init(oss.str());
        return pthis;
    }

public:

    /// @brief Parse the payload from the frame.
    /**
    The provided argument should support the `parse()` method.

    @param[out] payload The frame data payload to parse.
    @return `true` if data payload successfully parsed.
    */
    template<typename PayloadT>
    bool getPayload(PayloadT & payload) const
    {
        if (HEADER_LEN+FOOTER_LEN <= m_content.size())
        {
            const String pdata(
                m_content.begin()+HEADER_LEN, // skip signature and length
                m_content.end()-FOOTER_LEN);  // skip checksum
            IStringStream iss(pdata);
            io::BinaryIStream bs(iss);
            return payload.parse(bs);
        }

        return false; // empty
    }

public:

    ///@brief The frame parse result.
    enum ParseResult
    {
        RESULT_SUCCESS,     ///< @brief Frame succcessfully parsed.
        RESULT_INCOMPLETE,  ///< @brief Need more data.
        RESULT_BADCHECKSUM  ///< @brief Bad checksum found.
    };


    /// @brief Assign the frame content.
    /**
    This method searches frame signature, veryfies the checksum
    and assigns the frame content.

    @param[in] first The begin of input data.
    @param[in] last The end of input data.
    @param[out] n_skip The number of bytes processed.
    @param[out] result The parse result. May be NULL.
    @return Parsed frame or NULL.
    */
    template<typename In>
    static SharedPtr parseFrame(In first, In last, size_t &n_skip, ParseResult *result)
    {
        ParseResult res = RESULT_INCOMPLETE;
        SharedPtr frame; // empty frame

        // search for signature
        for (; first != last; ++first, ++n_skip)
        {
            if (UInt8(*first) == SIGNATURE)
                break;
        }

        const size_t buf_len = std::distance(first, last);
        if (HEADER_LEN+0+FOOTER_LEN <= buf_len) // can parse header
        {
            const In s_first = first;

            first += SIGNATURE_LEN; // skip signature
            const int len_msb = UInt8(*first++);
            const int len_lsb = UInt8(*first++);
            const size_t len = (len_msb<<8) | len_lsb;

            if (HEADER_LEN+len+FOOTER_LEN <= buf_len) // can parse whole frame
            {
                if (checksum(first, first+(len+CHECKSUM_LEN)) != 0x00)
                {
                    // bad checksum
                    res = RESULT_BADCHECKSUM;
                    n_skip += SIGNATURE_LEN; // skip signature
                }
                else
                {
                    // assign the frame content
                    frame.reset(new Frame());
                    frame->m_content.assign(s_first,
                        s_first+(HEADER_LEN+len+FOOTER_LEN));
                    n_skip += HEADER_LEN+len+FOOTER_LEN;
                    res = RESULT_SUCCESS;
                }
            }
            //else incomplete
        }
        // else incomplete

        if (result)
            *result = res;
        return frame;
    }


    /// @brief Assign the frame content from stream buffer.
    /**
    This method searches frame signature, veryfies the checksum
    and assigns the frame content.

    @param[in,out] sb The input data buffer.
    @param[out] result The parse result. May be NULL.
    @return Parsed frame or NULL.
    */
    static SharedPtr parseFrame(boost::asio::streambuf & sb, ParseResult *result)
    {
        size_t n_skip = 0; // number of bytes to skip
        const boost::asio::streambuf::const_buffers_type bufs = sb.data();
        SharedPtr frame = parseFrame(boost::asio::buffers_begin(bufs),
            boost::asio::buffers_end(bufs), n_skip, result);
        sb.consume(n_skip);
        return frame;
    }

public:

    /// @brief Get the frame intent (or command identifier).
    /**
    @return The frame intent or `-1` if it's unknown.
    */
    int getIntent() const
    {
        if (HEADER_LEN+1+FOOTER_LEN <= m_content.size())
            return m_content[HEADER_LEN];

        return -1; // unknown
    }

protected:

    /// @brief Calculate the checksum byte.
    /**
    @param[in] first The begin of frame data payload.
    @param[in] last The end of frame data payload.
    @return The checksum byte.
    */
    template<typename In>
    static UInt8 checksum(In first, In last)
    {
        int cs = 0;
        for (; first != last; ++first)
            cs += UInt8(*first);
        return UInt8(0xFF - (cs&0xFF));
    }


    /// @brief Initialize frame content from payload data.
    /**
    @param[in] payload The frame payload data.
    */
    void init(String const& payload)
    {
        const size_t len = payload.size();
        assert(len <= 64*1024 && "frame data payload too big");

        m_content.reserve(HEADER_LEN+len+FOOTER_LEN);

        // header (signature+length)
        m_content.push_back(SIGNATURE);
        m_content.push_back((len>>8)&0xFF); // length, MSB
        m_content.push_back((len)&0xFF);    // length, LSB

        // payload
        m_content.insert(m_content.end(),
            payload.begin(), payload.end());

        // checksum
        m_content.push_back(
            checksum(payload.begin(),
                payload.end()));
    }
};


#if 1 // payloads

/// @brief The empty payload.
class Frame::Payload
{
public:

    /// @brief The default constructor.
    Payload()
    {}

public:

    /// @brief Format the payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {}


    /// @brief Parse the payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        return true;
    }

protected:

    /// @brief Get all remaining data.
    /**
    This method reads the data till the end of stream.

    @param[in,out] bs The input binary stream.
    @return The parsed data.
    */
    static String getAll(io::BinaryIStream & bs)
    {
        String data;
        while (!bs.getStream().eof())
        {
            const UInt8 b = bs.getUInt8();
            if (!bs.getStream().eof())
                data.push_back(b);
            else
                break;
        }
        return data;
    }
};


/// @brief The AT command request payload.
class Frame::ATCommandRequest:
    public Frame::Payload
{
public:
    UInt8 frameId; ///< @brief The frame identifier.
    String command; ///< @brief The AT command and parameters.

public:

    /// @brief The main constructor.
    /**
    @param[in] command_ The AT command (and parameters if any).
    @param[in] frameId_ The frame identifier.
    */
    ATCommandRequest(String command_, UInt8 frameId_)
        : frameId(frameId_), command(command_)
    {}


    /// @brief The default constructor.
    ATCommandRequest()
        : frameId(0)
    {}

public:

    /// @brief Format the AT command request payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {
        bs.putUInt8(Frame::ATCOMMAND_REQUEST);
        bs.putUInt8(frameId);
        bs.putBuffer(command.data(),
            command.size());
    }


    /// @brief Parse the AT command request payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        if (bs.getUInt8() != Frame::ATCOMMAND_REQUEST)
            return false; // bad frame type

        frameId = bs.getUInt8();
        command = getAll(bs);

        return true;
    }
};


/// @brief The AT command response payload.
class Frame::ATCommandResponse:
    public Frame::Payload
{
public:
    UInt8 frameId; ///< @brief The frame identifier.
    String command; ///< @brief The AT command.
    UInt8 status; ///< @brief The command status.
    String result; ///< @brief The command result

public:

    /// @brief The default constructor.
    ATCommandResponse()
        : frameId(0),
          status(0)
    {}

public:

    /// @brief Format the AT command response payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {
        bs.putUInt8(Frame::ATCOMMAND_RESPONSE);
        bs.putUInt8(frameId);
        bs.putBuffer(command.data(),
            command.size());
        bs.putUInt8(status);
        bs.putBuffer(result.data(),
            result.size());
    }


    /// @brief Parse the AT command response payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        if (bs.getUInt8() != Frame::ATCOMMAND_RESPONSE)
            return false; // bad frame type

        frameId = bs.getUInt8();

        command.clear();
        command.push_back(bs.getUInt8());
        command.push_back(bs.getUInt8());

        status = bs.getUInt8();
        result = getAll(bs);

        return true;
    }
};


/// @brief The ZigBee Transmit Request payload.
class Frame::ZBTransmitRequest:
    public Frame::Payload
{
public:
    UInt8 frameId; ///< @brief The frame identifier.
    UInt64 dstAddr64; ///< @brief The destination address (64 bits).
    UInt16 dstAddr16; ///< @brief The destination network address (16 bits).
    UInt8 bcastRadius; ///< @brief The broadcast radius.
    UInt8 options; ///< @brief The transmision options.
    String data; ///< @brief The data to send.

public:

    /// @brief The default constructor.
    ZBTransmitRequest()
        : frameId(0),
          dstAddr64(0),
          dstAddr16(0),
          bcastRadius(0),
          options(0)
    {}


    /// @brief The main constructor.
    /**
    @param[in] data_ The data to send.
    @param[in] frameId_ The frame identifier.
    @param[in] dstAddr64_ The destination address (64 bits).
    @param[in] dstAddr16_ The destination network address (16 bits).
    @param[in] bcastRadius_ The broadcast radius.
    @param[in] options_ The transmision options.
    */
    ZBTransmitRequest(String const& data_, UInt8 frameId_,
        UInt64 dstAddr64_ = 0xFFFF, UInt16 dstAddr16_ = 0xFFFE,
        UInt8 bcastRadius_ = 0, UInt8 options_ = 0)
        : frameId(frameId_),
          dstAddr64(dstAddr64_),
          dstAddr16(dstAddr16_),
          bcastRadius(bcastRadius_),
          options(options_),
          data(data_)
    {}

public:

    /// @brief Format the ZigBee Transmit Request payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {
        bs.putUInt8(Frame::ZB_TRANSMIT_REQUEST);
        bs.putUInt8(frameId);
        bs.putUInt64(misc::h2be(dstAddr64));
        bs.putUInt16(misc::h2be(dstAddr16));
        bs.putUInt8(bcastRadius);
        bs.putUInt8(options);
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @brief Parse the ZigBee Transmit Request payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        if (bs.getUInt8() != Frame::ZB_TRANSMIT_REQUEST)
            return false;

        frameId = bs.getUInt8();
        dstAddr64 = misc::be2h(bs.getUInt64());
        dstAddr16 = misc::be2h(bs.getUInt16());
        bcastRadius = bs.getUInt8();
        options = bs.getUInt8();
        data = getAll(bs);

        return true;
    }
};


/// @brief The ZigBee Transmit Status payload.
class Frame::ZBTransmitStatus:
    public Frame::Payload
{
public:
    UInt8 frameId; ///< @brief The frame identifier.
    UInt16 dstAddr16; ///< @brief The destination network address (16 bits).
    UInt8 retryCount; ///< @brief The number of retries.
    UInt8 deliveryStatus; ///< @brief The delivery status.
    UInt8 discoveryStatus; ///< @brief The discovery status.

public:

    /// @brief The default constructor.
    ZBTransmitStatus()
        : frameId(0),
          dstAddr16(0),
          retryCount(0),
          deliveryStatus(0),
          discoveryStatus(0)
    {}

public:

    /// @brief Format the ZigBee Transmit Status payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {
        bs.putUInt8(Frame::ZB_TRANSMIT_STATUS);
        bs.putUInt8(frameId);
        bs.putUInt16(misc::h2be(dstAddr16));
        bs.putUInt8(retryCount);
        bs.putUInt8(deliveryStatus);
        bs.putUInt8(discoveryStatus);
    }


    /// @brief Parse the ZigBee Transmit Status payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        if (bs.getUInt8() != Frame::ZB_TRANSMIT_STATUS)
            return false;

        frameId = bs.getUInt8();
        dstAddr16 = misc::be2h(bs.getUInt16());
        retryCount = bs.getUInt8();
        deliveryStatus = bs.getUInt8();
        discoveryStatus = bs.getUInt8();

        return true;
    }
};


/// @brief The ZigBee Receive Packet payload.
class Frame::ZBReceivePacket:
    public Frame::Payload
{
public:
    UInt64 srcAddr64; ///< @brief The source address (64 bits).
    UInt16 srcAddr16; ///< @brief The source network address (16 bits).
    UInt8 options;  ///< @brief The receive options.
    String data;    ///< @brief The data string.

public:

    /// @brief The default constructor.
    ZBReceivePacket()
        : srcAddr64(0),
          srcAddr16(0),
          options(0)
    {}

public:

    /// @brief Format the ZigBee Receive Packet payload.
    /**
    @param[in,out] bs The output binary stream.
    */
    void format(io::BinaryOStream & bs) const
    {
        bs.putUInt8(Frame::ZB_RECEIVE_PACKET);
        bs.putUInt64(misc::h2be(srcAddr64));
        bs.putUInt16(misc::h2be(srcAddr16));
        bs.putUInt8(options);
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @brief Parse the ZigBee Receive Packet payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(io::BinaryIStream & bs)
    {
        if (bs.getUInt8() != Frame::ZB_RECEIVE_PACKET)
            return false;

        srcAddr64 = misc::be2h(bs.getUInt64());
        srcAddr16 = misc::be2h(bs.getUInt16());
        options = bs.getUInt8();
        data = getAll(bs);

        return true;
    }
};

#endif // payloads


/// @brief The XBee debug interface.
class Debug
{
public:

    /// @brief Dump the AT command request to string.
    /**
    @param[in] payload The payload to dump.
    @return The dump information.
    */
    static String dump(Frame::ATCommandRequest const& payload)
    {
        OStringStream oss;

        oss << "frameId=" << int(payload.frameId) << " "
            "command=\"" << payload.command << "\"";

        return oss.str();
    }


    /// @brief Dump the AT command response to string.
    /**
    @param[in] payload The payload to dump.
    @return The dump information.
    */
    static String dump(Frame::ATCommandResponse const& payload)
    {
        OStringStream oss;

        oss << "frameId=" << int(payload.frameId)
            << " command=\"" << payload.command << "\""
            << " status=" << dump::hex(payload.status)
            << " result=[" << dump::hex(payload.result) << "]";

        return oss.str();
    }


    /// @brief Dump the ZB transmit request to string.
    /**
    @param[in] payload The payload to dump.
    @return The dump information.
    */
    static String dump(Frame::ZBTransmitRequest const& payload)
    {
        OStringStream oss;

        oss << "frameId=" << int(payload.frameId)
            << " DA64=" << dump::hex(payload.dstAddr64)
            << " DA16=" << dump::hex(payload.dstAddr16)
            << " bcastRadius=" << int(payload.bcastRadius)
            << " options=" << dump::hex(payload.options)
            << " data=[" << dump::hex(payload.data)
            << "] (ascii:\"" << dump::ascii(payload.data) << "\")";

        return oss.str();
    }


    /// @brief Dump the ZB transmit status to string.
    /**
    @param[in] payload The payload to dump.
    @return The dump information.
    */
    static String dump(Frame::ZBTransmitStatus const& payload)
    {
        OStringStream oss;

        oss << "frameId=" << int(payload.frameId)
            << " DA16=" << dump::hex(payload.dstAddr16)
            << " retryCount=" << int(payload.retryCount)
            << " delivery=" << dump::hex(payload.deliveryStatus)
            << " discovery=" << dump::hex(payload.discoveryStatus);

        return oss.str();
    }


    /// @brief Dump the ZB receive packet to string.
    /**
    @param[in] payload The payload to dump.
    @return The dump information.
    */
    static String dump(Frame::ZBReceivePacket const& payload)
    {
        OStringStream oss;

        oss << "SA64=" << dump::hex(payload.srcAddr64)
            << " SA16=" << dump::hex(payload.srcAddr16)
            << " options=" << dump::hex(payload.options)
            << " data=[" << dump::hex(payload.data)
            << "] (ascii:\"" << dump::ascii(payload.data) << "\")";

        return oss.str();
    }

public:

    /// @brief Dump the frame to string.
    /**
    @param[in] frame The frame to dump.
    @return The dump information.
    */
    static String dump(Frame::SharedPtr frame)
    {
        OStringStream oss;

        if (frame)
        switch (frame->getIntent())
        {
            case Frame::ATCOMMAND_REQUEST:      tdump<Frame::ATCommandRequest>(oss, frame, "ATCOMMAND_REQUEST");    break;
            case Frame::ATCOMMAND_RESPONSE:     tdump<Frame::ATCommandResponse>(oss, frame, "ATCOMMAND_RESPONSE");  break;
            case Frame::ZB_TRANSMIT_REQUEST:    tdump<Frame::ZBTransmitRequest>(oss, frame, "ZB_TRANSMIT_REQUEST"); break;
            case Frame::ZB_TRANSMIT_STATUS:     tdump<Frame::ZBTransmitStatus>(oss, frame, "ZB_TRANSMIT_STATUS");   break;
            case Frame::ZB_RECEIVE_PACKET:      tdump<Frame::ZBReceivePacket>(oss, frame, "ZB_RECEIVE_PACKET");     break;

            default:
                oss << "unknown frame type: " << frame->getIntent();
                break;
        }

        return oss.str();
    }

private:

    /// @brief Dump the custom payload.
    /**
    @param[in,out] oss The output stream.
    @param[in] frame The frame content.
    @param[in] title The payload title.
    */
    template<typename PayloadT>
    static void tdump(OStream &oss, Frame::SharedPtr frame, const char *title)
    {
        oss << title << ": ";
        PayloadT payload;
        if (frame->getPayload(payload))
            oss << dump(payload);
        else
            oss << "bad format";
    }
};


/// @brief The XBee interface.
/**
Uses external stream object to communicate with XBee device.
Application is responsible to open and setup serial device.
*/
template<typename StreamT>
class API:
    public bin::Transceiver<StreamT, Frame>
{
    /// @brief The base type.
    typedef bin::Transceiver<StreamT, Frame> Base;

    /// @brief The type alias.
    typedef API<StreamT> This;

private:

    /// @brief The default constructor.
    /**
    @param[in] stream The external stream.
    */
    explicit API(StreamT &stream)
        : Base("xbee/API", stream)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<This> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] stream The external stream.
    @return The new instance.
    */
    static SharedPtr create(StreamT &stream)
    {
        return SharedPtr(new This(stream));
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<This>(Base::shared_from_this());
    }
};

} // xbee namespace

#endif // __DEVICEHIVE_XBEE_HPP_
