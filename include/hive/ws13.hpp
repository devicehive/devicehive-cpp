/** @file
@brief The WebSocket classes.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

The protocol version is 13.

@see @ref page_hive_http
@see @ref page_hive_ws13
@see [RFC6455](http://tools.ietf.org/html/rfc6455)
*/
#ifndef __HIVE_WS13_HPP_
#define __HIVE_WS13_HPP_

#include "http.hpp"
#include "bin.hpp"

//#include <boost/random/mersenne_twister.hpp>
#include <boost/uuid/sha1.hpp>

namespace hive
{
    /// @brief The WebSocket module.
    /**
    This namespace contains classes and functions
    related to WebSocket communication.
    */
    namespace ws13
    {

/// @brief The UUID used to indicate WebSocket protocol.
const char KEY_UUID[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/// @brief The protocol version.
const char VERSION[] = "13";

/// @brief The upgrade name.
const char NAME[] = "websocket";


        /// @brief The WebSocket headers.
        /**
        This namespace contains definition of common WebSocket headers.
        */
        namespace header
        {

const char Accept[]     = "Sec-WebSocket-Accept";       ///< @hideinitializer @brief The "Sec-WebSocket-Accept" header name.
const char Extensions[] = "Sec-WebSocket-Extensions";   ///< @hideinitializer @brief The "Sec-WebSocket-Extensions" header name.
const char Protocol[]   = "Sec-WebSocket-Protocol";     ///< @hideinitializer @brief The "Sec-WebSocket-Protocol" header name.
const char Version[]    = "Sec-WebSocket-Version";      ///< @hideinitializer @brief The "Sec-WebSocket-Version" header name.
const char Key[]        = "Sec-WebSocket-Key";          ///< @hideinitializer @brief The "Sec-WebSocket-Key" header name.

        } // header namespace


/// @brief The octet string.
/**
Elements of this string should be interpreted as octets not as characters.
*/
typedef String OctetString;


/// @brief The custom message.
/**
The message could be fragmented into multiple frames.
*/
class Message
{
protected:

    /// @brief The main constructor.
    /**
    @param[in] data The message payload data.
    @param[in] isText The "text" indicator.
    */
    explicit Message(OctetString const& data, bool isText)
        : m_data(data) , m_isText(isText)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Message> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] data The message payload data.
    @param[in] isText The "text" indicator.
    `true` for text data, `false` for binary data.
    @return The new message instance.
    */
    static SharedPtr create(OctetString const& data, bool isText = true)
    {
        return SharedPtr(new Message(data, isText));
    }

public:

    /// @brief Append data to the end of message.
    /**
    @param[in] data The data to append.
    */
    void appendData(OctetString const& data)
    {
        m_data.append(data);
    }

public:

    /// @brief Get the data.
    /**
    @return The text or binary octet string.
    */
    OctetString const& getData() const
    {
        return m_data;
    }


    /// @brief Get the "text" indicator.
    /**
    @return `true` for text data, `false` for binary.
    */
    bool isText() const
    {
        return m_isText;
    }

private:
    OctetString m_data; ///< @brief The data payload.
    bool m_isText; ///< @brief The "text" indicator.
};


/// @brief The WebSocket frame.
/**
This class contains full formated frame:

|     field | size in bytes |
|-----------|---------------|
|   control | 1             |
|    length | 1 or 3 or 9   |
|     mask  | 0 or 4        |
|   payload | N             |

The empty frame doesn't contain any data.

The following non-control payloads may be used:
  - Continue
  - Text
  - Binary
as well as control payloads:
  - Close
  - Ping
  - Pong
*/
class Frame:
    public bin::FrameContent
{
public:

    /// @brief The frame opcodes.
    enum Opcode
    {
        // non-control frames
        FRAME_CONTINUE  = 0x00, ///< @brief The *CONTINUE* frame opcode.
        FRAME_TEXT      = 0x01, ///< @brief The *TEXT* frame opcode.
        FRAME_BINARY    = 0x02, ///< @brief The *BINARY* frame opcode.

        // control frames
        FRAME_CLOSE     = 0x08, ///< @brief The *CLOSE* frame opcode.
        FRAME_PING      = 0x09, ///< @brief The *PING* frame opcode.
        FRAME_PONG      = 0x0A  ///< @brief The *PONG* frame opcode.
    };

public: // payloads

    class Payload;
        class Continue;
        class Text;
        class Binary;
        class Close;
        class Ping;
        class Pong;

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
    Should be valid payload with `format()` method.
    @param[in] masking The masking flag.
    @param[in] mask The masking key. Ignored if @a masking is `false`.
    @param[in] FIN The last frame indicator. `true` for the last frame.
    @param[in] flags The 3 reserved bits.
    @return The new frame instance.
    */
    template<typename PayloadT>
    static SharedPtr create(PayloadT const& payload, bool masking,
        UInt32 mask = 0, bool FIN = true, int flags = 0)
    {
        OStringStream oss;
        bin::OStream bs(oss);
        payload.format(bs);

        SharedPtr pthis(new Frame());
        pthis->init(oss.str(), PayloadT::OPCODE,
            masking, mask, FIN, flags);
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
        if (2 <= m_content.size())
        {
            OctetString frame(m_content.begin(), m_content.end());
            IStringStream iss(frame);
            bin::IStream bs(iss);

            const int f_ctl = bs.getUInt8();
            const int f_len = bs.getUInt8();
            const bool masking = (f_len&0x80) != 0;
            const int opcode = f_ctl&0x0F;
            (void)opcode; // mark as used

            assert(opcode == PayloadT::OPCODE
                && "invalid payload opcode");

            size_t len = f_len&0x7F;
            if (127 == len)         // UInt64
                len = (size_t)bs.getUInt64BE();
            else if (126 == len)    // UInt16
                len = bs.getUInt16BE();

            if (masking)
            {
                const UInt32 mask = bs.getUInt32BE();
                const size_t offset = (size_t)iss.tellg();
                for (size_t i = 0; i < len; ++i)
                {
                    const size_t k = (3 - (i%4));
                    const UInt8 M = mask >> (k*8);
                    frame[offset+i] ^= M;
                }

                // re-init input stream
                IStringStream iss(frame);
                bin::IStream bs(iss);
                iss.seekg(offset);
                return payload.parse(bs);
            }
            else
                return payload.parse(bs);
        }

        return false; // empty
    }

public:

    /// @brief Get the frame opcode.
    /**
    @return The frame opcode or `-1` if it's unknown.
    */
    int getOpcode() const
    {
        if (1 <= m_content.size())
        {
            const int f_ctl = UInt8(m_content[0]);
            return f_ctl&0x0F;
        }

        return -1; // unknown
    }


    /// @brief Get the frame FIN flag.
    /**
    @return The frame FIN flag.
        `-1` for empty frame.
    */
    int getFIN() const
    {
        if (1 <= m_content.size())
        {
            const int f_ctl = UInt8(m_content[0]);
            return (f_ctl>>7) != 0;
        }

        return -1; // unknown
    }


    /// @brief Get the frame flags.
    /**
    @return The frame flags. 3 bits.
        `-1` for empty frame.
    */
    int getFlags() const
    {
        if (1 <= m_content.size())
        {
            const int f_ctl = UInt8(m_content[0]);
            return (f_ctl>>4)&0x07;
        }

        return -1; // unknown
    }

public:

    ///@brief The frame parse result.
    enum ParseResult
    {
        RESULT_SUCCESS,    ///< @brief Frame succcessfully parsed.
        RESULT_INCOMPLETE  ///< @brief Need more data.
    };


    /// @brief Assign the frame content.
    /**
    This method assigns the frame content.

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
        SharedPtr frame; // no frame

        const size_t buf_len = std::distance(first, last);
        if (2 <= buf_len) // can parse header
        {
            const In s_first = first;
            const int f_ctl = UInt8(*first++);
            const int f_len = UInt8(*first++);
            const bool masking = (f_len&0x80) != 0;
            size_t h_len = 2 + (masking?4:0);   // header length
            size_t len = f_len&0x7F;            // payload length
            (void)f_ctl; // mark as used

            if (127 == len)             // UInt64 length
            {
                if (2+8 <= buf_len)
                {
                    UInt64 len_ex = UInt8(*first++); // MSB-first
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);
                    len_ex = (len_ex<<8) | UInt8(*first++);

                    len = size_t(len_ex);
                    if (UInt64(len) != len_ex) // TODO: limit the frame size
                        throw std::runtime_error("frame too big");
                    h_len += 8;
                }
                else
                    len = buf_len; // mark as incomplete
            }
            else if (126 == len)        // UInt16 length
            {
                if (2+2 <= buf_len)
                {
                    UInt16 len_ex = UInt8(*first++); // MSB-first
                    len_ex = (len_ex<<8) | UInt8(*first++);

                    len = len_ex;
                    h_len += 2;
                }
                else
                    len = buf_len; // mark as incomplete
            }

            if (h_len+len <= buf_len) // can parse whole frame
            {
                // assign the frame content
                frame.reset(new Frame());
                frame->m_content.assign(s_first,
                    s_first + (h_len+len));
                n_skip += (h_len+len);
                res = RESULT_SUCCESS;
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

protected:

    /// @brief Initialize frame content from payload data.
    /**
    @param[in] payload The frame payload data.
    @param[in] opcode The frame opcode.
    @param[in] masking The masking flag.
    @param[in] mask The masking key.
    @param[in] FIN The FIN flag.
    @param[in] flags The reserved flags (3 bits).
    */
    void init(OctetString const& payload, int opcode,
        bool masking, UInt32 mask, bool FIN, int flags)
    {
        boost::asio::streambuf sbuf;
        OStream os(&sbuf);
        bin::OStream bs(os);

        const size_t len = payload.size();

        // frame control field
        bs.putUInt8(((FIN?1:0)<<7) | ((flags&0x07)<<4) | (opcode&0x0F));

        // frame length field
        if (len < 126)                  // simple length
        {
            bs.putUInt8(((masking?1:0)<<7) | len);
        }
        else if (len < 64*1024)         // 2 extended bytes
        {
            bs.putUInt8(((masking?1:0)<<7) | 126);
            bs.putUInt16BE(len);
        }
        else                            // 8 extended bytes
        {
            bs.putUInt8(((masking?1:0)<<7) | 127);
            bs.putUInt64BE(len);
        }

        if (masking)
        {
            bs.putUInt32BE(mask);

            // mask the payload
            for (size_t i = 0; i < len; ++i)
            {
                const size_t k = (3 - (i%4));
                const UInt8 M = mask >> (k*8);
                bs.putUInt8(payload[i] ^ M);
            }
        }
        else
        {
            // do not mask the payload, just copy
            bs.putBuffer(payload.data(), payload.size());
        }

        boost::asio::streambuf::const_buffers_type buf = sbuf.data();
        m_content.assign(boost::asio::buffers_begin(buf),
            boost::asio::buffers_end(buf));
    }
};


/// @brief The empty payload.
/**
This is base class for all frame payloads.
*/
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
    void format(bin::OStream & bs) const
    {}


    /// @brief Parse the payload.
    /**
    @param[in,out] bs The input binary stream.
    @return `true` if successfully parsed.
    */
    bool parse(bin::IStream & bs)
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
    static OctetString getAll(bin::IStream & bs)
    {
        OStringStream oss;
        oss << bs.getStream().rdbuf();
        return oss.str();
    }
};


/// @brief The Continue frame payload.
/**
This payload is used as *CONTINUE* i.e. *non-first* frame
for fragmented messages (both *TEXT* and *BINARY*).
*/
class Frame::Continue:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_CONTINUE
    };

public:
    OctetString data; ///< @brief The custom data.

public:

    /// @brief The main/default constructor.
    /**
    @param[in] data_ The custom data.
    */
    explicit Continue(OctetString data_ = OctetString())
        : data(data_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        data = getAll(bs);
        return true;
    }
};


/// @brief The *TEXT* frame payload.
/**
This payload is used for *TEXT* frames.
*/
class Frame::Text:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_TEXT
    };

public:
    String text; ///< @brief The text (UTF-8).

public:

    /// @brief The main/default constructor.
    /**
    @param[in] text_ The text.
    */
    explicit Text(String text_ = String())
        : text(text_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putBuffer(text.data(),
            text.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        text = getAll(bs);
        return true;
    }
};


/// @brief The *BINARY* frame payload.
/**
This payload is used for *BINARY* frames.
*/
class Frame::Binary:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_BINARY
    };

public:
    OctetString data; ///< @brief The custom data.

public:

    /// @brief The main/default constructor.
    /**
    @param[in] data_ The custom data.
    */
    explicit Binary(OctetString data_ = OctetString())
        : data(data_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        data = getAll(bs);
        return true;
    }
};


/// @brief The *CLOSE* frame payload.
/**
This payload is used to indicate connection closure.
*/
class Frame::Close:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_CLOSE
    };


    /// @brief The status codes.
    enum StatusCode
    {
        /// @brief Indicates a normal closure.
        STATUS_NORMAL           = 1000,

        /// @brief Indicates that an endpoint is "going away".
        STATUS_GOING_AWAY       = 1001,

        /// @brief Indicates that an endpoint is terminating the connection
        /// due to a protocol error.
        STATUS_PROTOCOL_ERROR   = 1002,

        /// @brief Indicates that an endpoint is terminating the connection
        /// because it has received a type of data it cannot accept.
        STATUS_UNEXPECTED_DATA  = 1003,

        /// @brief Indicates that an endpoint is terminating the connection
        /// because it has received data within a message that was
        /// not consistent with the type of the message.
        STATUS_INVALID_DATA     = 1007,

        /// @brief Indicates that an endpoint is terminating the connection
        /// because it has received a message that is too big for it to process.
        STATUS_MESSAGE_TOO_BIG  = 1009,

        /// @brief Indicates that an endpoint (client) is terminating the
        /// connection because it has expected the server to negotiate one
        /// or more extension.
        STATUS_NO_EXTENSION     = 1010,

        /// @brief Indicates that a server is terminating the connection because
        /// it encountered an unexpected condition that prevented it from
        /// fulfilling the request.
        STATUS_UNEXPECTED_COND  = 1011
    };

public:
    UInt16 statusCode; ///< @brief The optional status code.
    String reason; ///< @brief The optional reason phrase.

public:

    /// @brief The main/default constructor.
    /**
    @param[in] statusCode_ The status code.
    @param[in] reason_ The reason phrase.
    */
    explicit Close(UInt16 statusCode_ = STATUS_NORMAL, String reason_ = String())
        : statusCode(statusCode_), reason(reason_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putUInt16BE(statusCode);
        bs.putBuffer(reason.data(),
            reason.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        statusCode = bs.getUInt16BE(); // optional
        reason = getAll(bs); // up to end
        return true;
    }
};


/// @brief The *PING* frame payload.
/**
This payload is used to theck if peer is alive.
*/
class Frame::Ping:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_PING
    };

public:
    OctetString data; ///< @brief The application data.

public:

    /// @brief The main/default constructor.
    /**
    @param[in] data_ The application data.
    */
    explicit Ping(OctetString data_ = OctetString())
        : data(data_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        data = getAll(bs); // up to end
        return true;
    }
};


/// @brief The *PONG* frame payload.
/**
This payload could be used as a response to the *PING* request
or as a standalone *heartbeat* message.
*/
class Frame::Pong:
    public Frame::Payload
{
public:

    enum
    {
        /// @brief The opcode identifier.
        OPCODE = Frame::FRAME_PONG
    };

public:
    OctetString data; ///< @brief The application data.

public:

    /// @brief The main/default constructor.
    /**
    @param[in] data_ The application data.
    */
    explicit Pong(OctetString data_ = OctetString())
        : data(data_)
    {}

public:

    /// @copydoc Frame::Payload::format()
    void format(bin::OStream & bs) const
    {
        bs.putBuffer(data.data(),
            data.size());
    }


    /// @copydoc Frame::Payload::parse()
    bool parse(bin::IStream & bs)
    {
        data = getAll(bs); // up to end
        return true;
    }
};


        // implementation
        namespace impl
        {

/// @brief Get the SHA-1 digest.
/**
@param[in] data The custom binary data.
@return The SHA-1 hash.
*/
inline OctetString sha1(OctetString const& data)
{
    boost::uuids::detail::sha1 h;
    h.process_bytes(data.data(),
        data.size());

    const size_t N = 5;
    unsigned int hash[N];
    h.get_digest(hash);

    OStringStream oss;
    bin::OStream bs(oss);
    for (size_t i = 0; i < N; ++i)
    {
        //oss << dump::hex(hash[i]);
        bs.putUInt32BE(hash[i]);
    }

    return oss.str();
}

        } // implementation


/// @brief The %WebSocket connection.
/**
Client socket connection. Sends masked frames.

It's possible to work with websocket messages (high level)
and websocket frames (low level).
*/
class WebSocket:
    public boost::enable_shared_from_this<WebSocket>
{
    typedef bin::Transceiver<http::Connection, Frame> TRX; ///< @brief The transceiver type.
    typedef WebSocket This; ///< @brief The this type alias.

protected:

    /// @brief The main constructor.
    /**
    @param[in] name The optional websocket name.
    */
    explicit WebSocket(String const& name)
        : m_log("/hive/websocket/" + name)
    {
        HIVELOG_TRACE_STR(m_log, "created");
    }

public:

    /// @brief The trivial destructor.
    ~WebSocket()
    {
        HIVELOG_TRACE_STR(m_log, "deleted");
    }

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<WebSocket> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] name The optional websocket name.
    @return The new websocket instance.
    */
    static SharedPtr create(String const& name = String())
    {
        return SharedPtr(new WebSocket(name));
    }

public:

    /// @brief Prepare websocket handshake request.
    /**
    This HTTP request might be used for manual connection.

    @param[in] url The URL to request.
    @param[in] key The random key (base64 encoded).
        Empty string for automatic key.
    @return The WebSocket handshake request.
    */
    static http::RequestPtr getHandshakeRequest(http::Url const& url, String const& key = String())
    {
        http::RequestPtr req = http::Request::GET(url);
        req->setVersion(1, 1); // at least HTTP 1.1 required
        req->addHeader(http::header::Connection, "Upgrade");
        req->addHeader(http::header::Upgrade, NAME);
        req->addHeader(ws13::header::Version, VERSION);
        req->addHeader(ws13::header::Key, key.empty() ? generateNewKey() : key);
        return req;
    }


    /// @brief Generate new handshake key.
    /**
    @return The new key, base64 encoded.
    */
    static String generateNewKey()
    {
        // TODO: boost::random::mt19937 rgen;
        const size_t N = 16;
        std::vector<UInt8> key(N);

        for (size_t i = 0; i < N; ++i)
            key[i] = rand()&0xFF;

        return http::base64_encode(key);
    }


    /// @brief Build handshake accept key.
    /**
    @param[in] key The websocket key.
    @return The handshake accept key.
    */
    static String buildAcceptKey(String const& key)
    {
        const OctetString check = impl::sha1(key + KEY_UUID);
        return http::base64_encode(check.begin(), check.end());
    }


    /// @brief Generate new masking key.
    /**
    @return The new masking key.
    */
    static UInt32 generateNewMask()
    {
        // TODO: boost::random::mt19937 rgen;
        return (rand()<<16) | rand();
    }

public:

    /// @brief The "connect" callback type.
    typedef boost::function2<void, boost::system::error_code, SharedPtr> ConnectCallback;


    /// @brief Connect to the server.
    /**
    @param[in] url The URL to connect to.
    @param[in] httpClient The corresponding HTTP client.
    @param[in] callback The "connect" callback will be called after creation.
    @param[in] timeout_ms Connection timeout in milliseconds.
    @param[in] key The websocket key, base64 encoded.
        Empty string for random key.
    */
    void asyncConnect(http::Url const& url, http::Client::SharedPtr httpClient,
        ConnectCallback callback, size_t timeout_ms, String const& key = String())
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncConnect()");

        HIVELOG_DEBUG(m_log, "try to connect to " << url.toString());
        httpClient->send2(getHandshakeRequest(url, key),
            boost::bind(&This::onConnect, shared_from_this(),
            _1, _2, _3, _4, httpClient, callback), timeout_ms);
    }


    /// @brief Is the websocket opened?
    /**
    @return `true` if it's opened.
    */
    bool isOpen() const
    {
        return m_trx && m_conn;
    }


    /// @brief Close the socket.
    /**
    If *force* flag is `false`, sends **CLOSE** frame.
    Otherwise just closes the socket.
    @param[in] force The *force* flag.
    */
    void close(bool force)
    {
        if (isOpen())
        {
            if (!force)
            {
                const UInt32 mask = generateNewMask();

                HIVELOG_TRACE(m_log, "sending CLOSE frame");
                Frame::SharedPtr frame = Frame::create(Frame::Close(Frame::Close::STATUS_NORMAL), true, mask);
                asyncSendFrame(frame, boost::bind(&This::onSendFrame,
                    shared_from_this(), _1, _2, SendFrameCallback()));
            }
            else
            {
                m_trx->recv(TRX::RecvFrameCallback());
                m_trx.reset();
                m_conn->close();
                m_conn.reset();
            }
        }
    }

private:

    /// @brief The connect operation completed.
    /**
    @param[in] err The error code.
    @param[in] request The HTTP request.
    @param[in] response The HTTP response.
    @param[in] connection The HTTP connection.
    @param[in] httpClient The HTTP client.
    @param[in] callback The callback.
    */
    void onConnect(boost::system::error_code err, http::RequestPtr request,
        http::ResponsePtr response, http::ConnectionPtr connection,
        http::ClientPtr httpClient, ConnectCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onConnect()");

        if (!err && request && response && connection)
        {
            // TODO: check for "101 Switching Protocols" status code

            // check the accept key
            const String akey = response->getHeader(header::Accept);
            const String rkey = request->getHeader(header::Key);
            if (akey == buildAcceptKey(rkey))
            {
                m_conn = connection;
                m_trx = TRX::create(m_log.getName(), *m_conn);
                HIVELOG_INFO(m_log, "connection created");

                // start listening ASAP
                m_trx->recv(boost::bind(&This::onRecvFrame,
                    shared_from_this(), _1, _2));
            }
            else
            {
                err = boost::asio::error::connection_refused; // TODO: use appropriate error code
                HIVELOG_ERROR(m_log, "bad key: " << rkey
                    << " doesn't match to " << akey);
            }
        }

        httpClient->getIoService().post(
            boost::bind(callback, err, shared_from_this()));
    }

public:

    /// @brief The "send message" callback type.
    typedef boost::function2<void, boost::system::error_code, Message::SharedPtr> SendMessageCallbackType;

    /// @brief The "receive message" callback type.
    typedef boost::function2<void, boost::system::error_code, Message::SharedPtr> RecvMessageCallbackType;


    /// @brief Send the message.
    /**
    @param[in] msg The message to send.
    @param[in] callback The callback.
    @param[in] fragmentSize The maximum fragment size.
        Zero to disable fragmentation.
    */
    void asyncSendMessage(Message::SharedPtr msg, SendMessageCallbackType callback, size_t fragmentSize = 0)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncSendMessage()");

        size_t msg_size = msg->getData().size();
        if (fragmentSize && fragmentSize < msg_size)                        // fragmentation enabled
        {
            OctetString::const_iterator first = msg->getData().begin();
            bool isFirstFrame = true;

            // send full-frames
            while (2*fragmentSize < msg_size)
            {
                const OctetString data(first, first + fragmentSize);
                const UInt32 mask = generateNewMask();

                HIVELOG_DEBUG(m_log, "send "
                    << (isFirstFrame?"first ":"")
                    << "fragment (" << fragmentSize << " bytes): "
                    << dump::hex(data));

                // "client-to-server" frames are always masked
                Frame::SharedPtr frame = isFirstFrame ? (msg->isText()
                    ? Frame::create(Frame::Text(data), true, mask, false)
                    : Frame::create(Frame::Binary(data), true, mask, false))
                    : Frame::create(Frame::Continue(data), true, mask, false);
                asyncSendFrame(frame, boost::bind(&This::onSendMessage,
                    shared_from_this(), _1, _2, msg, callback));
                isFirstFrame = false;

                msg_size -= fragmentSize;
                first += fragmentSize;
            }

            // split the rest into the two fragments
            const size_t F1 = (msg_size+1)/2; // ceil
            const size_t F2 = msg_size - F1;

            { // send the first fragment
                const OctetString data(first, first + F1);
                const UInt32 mask = generateNewMask();

                HIVELOG_DEBUG(m_log, "send "
                    << (isFirstFrame?"first ":"")
                    << "fragment (" << F1 << " bytes): "
                    << dump::hex(data));

                // "client-to-server" frames are always masked
                Frame::SharedPtr frame = isFirstFrame ? (msg->isText()
                    ? Frame::create(Frame::Text(data), true, mask, false)
                    : Frame::create(Frame::Binary(data), true, mask, false))
                    : Frame::create(Frame::Continue(data), true, mask, false);
                asyncSendFrame(frame, boost::bind(&This::onSendMessage,
                    shared_from_this(), _1, _2, msg, callback));
                isFirstFrame = false;

                msg_size -= F1;
                first += F1;
            }

            { // send the second fragment (and the last one)
                const OctetString data(first, first + F2);
                const UInt32 mask = generateNewMask();

                HIVELOG_DEBUG(m_log, "send " << (isFirstFrame?"first ":"")
                    << "fragment (" << F2 << " bytes): "
                    << dump::hex(data));

                // "client-to-server" frames are always masked
                Frame::SharedPtr frame =
                    Frame::create(Frame::Continue(data), true, mask, true);
                asyncSendFrame(frame, boost::bind(&This::onSendMessage,
                    shared_from_this(), _1, _2, msg, callback));

                msg_size -= F2;
                first += F2;
            }

            assert(0 == msg_size && "fragmentation doesn't work");
        }
        else                                                                // single frame
        {
            const OctetString &data = msg->getData();
            const UInt32 mask = generateNewMask();

            HIVELOG_DEBUG(m_log, "send single frame: " << msg_size << " bytes");

            // "client-to-server" frames are always masked
            Frame::SharedPtr frame = msg->isText()
                ? Frame::create(Frame::Text(data), true, mask)
                : Frame::create(Frame::Binary(data), true, mask);
            asyncSendFrame(frame, boost::bind(&This::onSendMessage,
                shared_from_this(), _1, _2, msg, callback));
        }
    }


    /// @brief Listen for the messages.
    /**
    @param[in] callback The callback functor which will be called
        each time new message received. Pass NULL to stop listening.
    */
    void asyncListenForMessages(RecvMessageCallbackType callback)
    {
        m_recvMsgCallback = callback;
    }

private:
    RecvMessageCallbackType m_recvMsgCallback; ///< @brief The "receive message" callback.
    Message::SharedPtr m_recvMsg; ///< @brief The assembling message or NULL.


    /// @brief The "send frame" callback for messages.
    /**
    @param[in] err The error code.
    @param[in] frame The sent frame.
    @param[in] msg The corresponding message.
    @param[in] callback The users callback.
    */
    void onSendMessage(boost::system::error_code err, Frame::SharedPtr frame,
        Message::SharedPtr msg, SendMessageCallbackType callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onSendMessage(frame)");

        if (err || frame->getFIN() == 1)
        {
            HIVELOG_DEBUG(m_log, "final frame confirmed, report to user");
            if (callback)
            {
                m_trx->getStream().get_io_service().post(
                    boost::bind(callback, err, msg));
            }
        }
        else
        {
            // TODO: report error in any case
            HIVELOG_DEBUG(m_log, "just wait for the final frame");
        }
    }


    /// @brief Report the new message to the user.
    /**
    @param[in] err The error code.
    @param[in] msg The corresponding message.
    */
    void doRecvMessage(boost::system::error_code err, Message::SharedPtr msg)
    {
        HIVELOG_TRACE_BLOCK(m_log, "doRecvMessage(msg)");

        if (m_recvMsgCallback)
        {
            HIVELOG_DEBUG(m_log, "report message to user");
            m_trx->getStream().get_io_service().post(
                boost::bind(m_recvMsgCallback, err, msg));
        }
        else
            HIVELOG_WARN(m_log, "no callback, message ignored");
    }

public:

    /// @brief The "send frame" callback type.
    typedef TRX::SendFrameCallback SendFrameCallback;

    /// @brief The "receive frame" callback type.
    typedef TRX::RecvFrameCallback RecvFrameCallback;


    /// @brief Send the frame asynchronously.
    /**
    @param[in] frame The frame to send.
    @param[in] callback The callback.
    */
    void asyncSendFrame(Frame::SharedPtr frame, SendFrameCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncSendFrame()");
        assert(isOpen() && "not connected");

        m_trx->send(frame,
            boost::bind(&This::onSendFrame,
            shared_from_this(), _1, _2, callback));
    }


    /// @brief Listen for received frames.
    /**
    @param[in] callback The callback functor or NULL to stop listening.
    */
    void asyncListenForFrames(RecvFrameCallback callback)
    {
        m_recvFrameCallback = callback;
    }

private:
    RecvFrameCallback m_recvFrameCallback; ///< @brief The "receive frame" callback.

    /// @brief The "send frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The sent frame.
    @param[in] callback The user's callback.
    */
    void onSendFrame(boost::system::error_code err, Frame::SharedPtr frame, SendFrameCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onSendFrame()");

        if (callback)
        {
            m_trx->getStream().get_io_service().post(
                boost::bind(callback, err, frame));
        }
    }


    /// @brief The "receive frame" callback.
    /**
    Assembles messages.
    @param[in] err The error code.
    @param[in] frame The received frame.
    */
    void onRecvFrame(boost::system::error_code err, Frame::SharedPtr frame)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onRecvFrame()");

        if (!err)
        {
            const int opcode = frame->getOpcode();
            bool frame_processed = false;

            if (m_recvFrameCallback)
            {
                m_trx->getStream().get_io_service().post(
                    boost::bind(m_recvFrameCallback, err, frame));
                frame_processed = true;
            }

            // handle control frames here
            switch (opcode)
            {
                case Frame::FRAME_CLOSE:
                {
                    HIVELOG_DEBUG(m_log, "got CLOSE frame");
                    close(true);
                } break;

                case Frame::FRAME_PING:
                {
                    Frame::Ping ping;
                    frame->getPayload(ping);
                    HIVELOG_DEBUG(m_log, "got PING frame");

                    const UInt32 mask = generateNewMask();
                    HIVELOG_TRACE(m_log, "sending PONG frame");
                    Frame::SharedPtr frame = Frame::create(Frame::Pong(ping.data), true, mask);
                    asyncSendFrame(frame, boost::bind(&This::onSendFrame,
                        shared_from_this(), _1, _2, SendFrameCallback()));
                } break;

                default:
                    break;
            }

            if (m_recvMsgCallback) // try to assemble message from frames
            {
                if (!m_recvMsg)
                {
                    if (opcode == Frame::FRAME_BINARY)
                    {
                        Frame::Binary info;
                        frame->getPayload(info);
                        m_recvMsg = Message::create(info.data, false);
                        frame_processed = true;
                    }
                    else if (opcode == Frame::FRAME_TEXT)
                    {
                        Frame::Text info;
                        frame->getPayload(info);
                        m_recvMsg = Message::create(info.text, true);
                        frame_processed = true;
                    }
                    else
                    {
                        HIVELOG_WARN(m_log, "unexpected frame, ignored");
                    }
                }
                else
                {
                    if (opcode == Frame::FRAME_CONTINUE)
                    {
                        Frame::Continue info;
                        frame->getPayload(info);
                        m_recvMsg->appendData(info.data);
                        frame_processed = true;
                    }
                    else
                    {
                        HIVELOG_WARN(m_log, "previous message is broken, ignored");
                        m_recvMsg.reset();
                    }
                }

                if (m_recvMsg && frame->getFIN()==1)
                {
                    doRecvMessage(boost::system::error_code(), m_recvMsg);
                    m_recvMsg.reset();
                }
            }

            if (!frame_processed)
                HIVELOG_WARN(m_log, "no callback, frame ignored");
        }
        else
        {
            HIVELOG_ERROR(m_log, "RECV error: [" << err << "] - " << err.message());

            if (m_recvFrameCallback)
            {
                m_trx->getStream().get_io_service().post(
                    boost::bind(m_recvFrameCallback, err, Frame::SharedPtr()));
            }

            doRecvMessage(err, Message::SharedPtr());
        }
    }

private:
    http::ConnectionPtr m_conn; ///< @brief The corresponding HTTP connection.
    TRX::SharedPtr m_trx; ///< @brief The tranceiver.
    hive::log::Logger m_log; ///< @brief The logger.
};


/// @brief The WebSocket shared pointer type.
typedef WebSocket::SharedPtr WebSocketPtr;

    } // ws13 namespace
} // hive namespace

#endif // __HIVE_WS13_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_ws13 WebSocket module

This is page is under construction!
===================================
*/
