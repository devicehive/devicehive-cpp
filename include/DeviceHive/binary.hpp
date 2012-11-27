/** @file
@brief The binary transceiver prototype (experimental).
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#ifndef __SANDBOX_BINARY_HPP_
#define __SANDBOX_BINARY_HPP_

#include <hive/bstream.hpp>
#include <hive/swab.hpp>
#include <hive/dump.hpp>
#include <hive/log.hpp>

#if !defined(HIVE_PCH)
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <vector>
#   include <deque>
#endif // HIVE_PCH


/// @brief The binary transceiver prototype (experimental).
namespace binary
{
    using namespace hive;

/// @brief The simple frame example.
/**
The main aim of this class is to demonstrate frame prototype for all other formats.

This class holds the whole frame content incuding header, payload, and checksum.
The empty frame doesn't contain any data.

The frame format is quite simple:

|     field | size in bytes
|-----------|--------------
| signature | 1
|    length | 1
|    intent | 2 (little-endian)
|   payload | N
|  checksum | 1
*/
class SimpleFrame
{
public:

    /// @brief Constants.
    enum Const
    {
        SIGNATURE = 0xFC, ///< @brief The signature.
    };

private:

    /// @brief Various lengths.
    enum Length
    {
        SIGNATURE_LEN = 1, ///< @brief The "signature" field length in bytes.
        LENGTH_LEN    = 1, ///< @brief The "length" field length in bytes.
        INTENT_LEN    = 2, ///< @brief The "intent" field length in bytes.
        CHECKSUM_LEN  = 1, ///< @brief The "checksum" field length in bytes.

        /// @brief The total header length in bytes.
        HEADER_LEN = SIGNATURE_LEN + LENGTH_LEN + INTENT_LEN,

        /// @brief The total footer length in bytes.
        FOOTER_LEN = CHECKSUM_LEN
    };

protected:

    /// @brief The default constructor.
    /**
    Constructs the empty frame.
    */
    SimpleFrame()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<SimpleFrame> SharedPtr;


    /// @brief Construct the frame from the payload.
    /**
    @param[in] intent The message intent.
    @param[in] payload The data payload.
    Should be byte container with `begin()` and `end()` methods.
    For example std::vector<char> or hive::String.
    @return The new frame instance.
    */
    template<typename PayloadT>
    static SharedPtr create(int intent, PayloadT const& payload)
    {
        SharedPtr pthis(new SimpleFrame());
        pthis->init(intent, payload);
        return pthis;
    }

public:

    /// @brief Parse the payload from the frame.
    /**
    @param[out] payload The data payload to store.
    Should be byte container with `assign()` method.
    For example std::vector<char> or hive::String.
    @return `true` if data payload successfully parsed.
    */
    template<typename PayloadT>
    bool getPayload(PayloadT & payload) const
    {
        if (HEADER_LEN+FOOTER_LEN <= m_content.size())
        {
            payload.assign(
                m_content.begin()+HEADER_LEN, // skip signature+length+intent
                m_content.end()-FOOTER_LEN);  // skip checksum
            return true;
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
        if (HEADER_LEN+FOOTER_LEN <= buf_len) // can parse header
        {
            const In s_first = first;

            first += SIGNATURE_LEN; // skip signature
            const size_t len = UInt8(*first++);
            first += INTENT_LEN; // skip intent

            if (size_t(HEADER_LEN+len+FOOTER_LEN) <= buf_len) // can parse whole frame
            {
                if (checksum(s_first, s_first+(HEADER_LEN+len+FOOTER_LEN)) != 0x00)
                {
                    // bad checksum
                    res = RESULT_BADCHECKSUM;
                    n_skip += SIGNATURE_LEN; // skip signature
                }
                else
                {
                    // assign the frame content
                    frame.reset(new SimpleFrame());
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


    /// @brief Get the frame content.
    /**
    @return The frame content.
    */
    std::vector<UInt8> const& getContent() const
    {
        return m_content;
    }

public:

    /// @brief Get the frame intent.
    /**
    @return The frame intent or `-1` if it's unknown.
    */
    int getIntent() const
    {
        if (HEADER_LEN+0+FOOTER_LEN <= m_content.size())
        {
            size_t offset = SIGNATURE_LEN+LENGTH_LEN;

            const int id_lsb = m_content[offset++];
            const int id_msb = m_content[offset++];

            return (id_msb<<8) | id_lsb;
        }

        return -1; // unknown
    }

public:

    /// @brief Is the frame empty?
    /**
    @return `true` if the frame is empty.
    */
    bool empty() const
    {
        return m_content.empty();
    }


    /// @brief Get the frame size.
    /**
    This size includes the header size, frame payload and checksum.
    @return The frame size in bytes.
    */
    size_t size() const
    {
        return m_content.size();
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
    @param[in] intent The payload intent.
    @param[in] payload The frame payload data.
    */
    template<typename PayloadT>
    void init(int intent, PayloadT const& payload)
    {
        const size_t len = payload.size();
        assert(len < 255 && "frame data payload too big");

        m_content.reserve(HEADER_LEN+len+FOOTER_LEN);

        // header (signature+length+intent)
        m_content.push_back(SIGNATURE);         // signature
        m_content.push_back(len&0xFF);          // length
        m_content.push_back((intent)&0xFF);     // intent, LSB
        m_content.push_back((intent>>8)&0xFF);  // intent, MSB

        // insert payload
        m_content.insert(m_content.end(),
            payload.begin(), payload.end());

        // checksum
        m_content.push_back(checksum(
            m_content.begin(),
            m_content.end()));
    }

protected:
    std::vector<UInt8> m_content; ///< @brief The frame content.
};


/// @brief The transceiver engine.
/**
Uses external stream object which may be serial port, tcp socket,
or something else supported by boost.asio.
*/
// TODO: add the RX/TX timeouts?
template<typename StreamT, typename FrameT>
class Transceiver:
    public boost::enable_shared_from_this< Transceiver<StreamT, FrameT> >
{
    /// @brief The type alias.
    typedef Transceiver<StreamT, FrameT> This;

protected:

    /// @brief The default constructor.
    /**
    @param[in] loggerName The logger name.
    @param[in] stream The external stream object.
    */
    explicit Transceiver(String const& loggerName, StreamT &stream)
        : m_rx_in_progress(false)
        , m_tx_in_progress(false)
        , m_stream(stream)
        , m_log(loggerName)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<This> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] loggerName The logger name.
    @param[in] stream The external stream.
    @return The new transceiver instance.
    */
    static SharedPtr create(String const& loggerName, StreamT &stream)
    {
        return SharedPtr(new This(loggerName, stream));
    }

public:

    /// @brief The external stream type.
    typedef StreamT Stream;

    /// @brief The frame type.
    typedef FrameT Frame;

    /// @brief The frame shared pointer type.
    typedef typename Frame::SharedPtr FrameSPtr;


    /// @brief The "recv" frame callback type.
    typedef boost::function2<void,
        boost::system::error_code,
        FrameSPtr> RecvFrameCallback;

    /// @brief The "send" frame callback type.
    typedef boost::function2<void,
        boost::system::error_code,
        FrameSPtr> SendFrameCallback;

public:

    /// @brief Get the external stream.
    /**
    @return The external stream reference.
    */
    Stream& getStream()
    {
        return m_stream;
    }

public:

    /// @brief Start listening for the RX frames.
    /**
    This callback will be called each time the new frame is received.
    To stop listening just pass the NULL pointer to this method.
    @param[in] callback The callback functor.
    */
    void recv(RecvFrameCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "recv()");
        HIVELOG_DEBUG(m_log, (callback ? "start":"stop")
            << " listening for the RX frames");

        m_rx_callback = callback;
        if (m_rx_callback)
            asyncReadSome();
    }


    /// @brief Send the custom frame.
    /**
    @param[in] frame The frame to send.
    @param[in] callback The callback functor.
    */
    void send(FrameSPtr frame, SendFrameCallback callback)
    {
        HIVELOG_TRACE_BLOCK(m_log, "send()");
        HIVELOG_DEBUG(m_log, "request to send frame: ["
            << hexdump(frame) << "]");

        SendTaskSPtr task(new SendTask(callback, frame));

        // start the TX task if possible
        m_tx_tasks.push_back(task);
        if (!m_tx_in_progress) // if no active task
            startNextTxTask();
    }

public:

    /// @brief Dump the buffer to string in HEX format.
    /**
    @param[in] buffer The buffer to dump.
    @return The buffer in HEX format.
    */
    template<typename ConstBufferSequence>
    static String hexdump(ConstBufferSequence const& buffer)
    {
        return dump::hex(
            boost::asio::buffers_begin(buffer),
            boost::asio::buffers_end(buffer));
    }


    /// @brief Dump the buffer to string in HEX format.
    /**
    @param[in] sb The stream buffer to dump.
    @return The buffer in HEX format.
    */
    static String hexdump(boost::asio::streambuf const& sb)
    {
        return hexdump(sb.data());
    }


    /// @brief Dump the frame to string in HEX format.
    /**
    @param[in] frame The frame to dump.
    @return The buffer in HEX format.
    */
    static String hexdump(FrameSPtr frame)
    {
        return frame ? dump::hex(frame->getContent()) : String();
    }

private:

    /// @brief Start or continue read operation.
    void asyncReadSome()
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncReadSome()");

        if (!m_rx_in_progress)
        {
            m_rx_in_progress = true;

            HIVELOG_DEBUG_STR(m_log, "start async read some");
            boost::asio::async_read(m_stream, m_rx_buf,
                boost::asio::transfer_at_least(1),
                boost::bind(&This::onReadSome, this->shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else
            HIVELOG_DEBUG_STR(m_log, "async read in progress, do nothing");
    }


    /// @brief The read operation completed.
    /**
    @param[in] err The error code.
    @param[in] len The number of bytes transfered.
    */
    void onReadSome(boost::system::error_code err, size_t len)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onReadSome()");
        HIVELOG_TRACE(m_log, "arguments: err="
            << err << ", len=" << len);

        m_rx_in_progress = false;

        if (!err)
        {
            HIVELOG_DEBUG(m_log, "read " << len
                << " bytes, RX buffer: ["
                << hexdump(m_rx_buf) << "]");

            while (1) // try to parse frames
            {
                typename Frame::ParseResult result = Frame::RESULT_SUCCESS;
                if (FrameSPtr frame = Frame::parseFrame(m_rx_buf, &result))
                {
                    HIVELOG_DEBUG(m_log, "new frame parsed: ["
                        << hexdump(frame) << "]");
                    done(err, frame);
                    // continue;
                }
                else
                {
                    if (result == Frame::RESULT_INCOMPLETE)
                    {
                        HIVELOG_DEBUG_STR(m_log, "not enough data to parse frame");
                        break; // stop parsing
                    }
                    else
                    {
                        HIVELOG_WARN(m_log, "frame parse result=" << result);
                        // continue;
                    }
                }
            }

            asyncReadSome(); // continue RX
        }
        else
        {
            if (err == boost::asio::error::operation_aborted)
                HIVELOG_DEBUG_STR(m_log, "read operation cancelled");
            else
                HIVELOG_ERROR(m_log, "read operation error: ["
                    << err << "] " << err.message());

            done(err, FrameSPtr());
        }
    }


    /// @brief Report new RX frame to the subscriber.
    /**
    @param[in] err The error code.
    @param[in] frame The received frame. May be NULL.
    */
    void done(boost::system::error_code err, FrameSPtr frame)
    {
        HIVELOG_TRACE_BLOCK(m_log, "done(rx)");

        if (m_rx_callback)
        {
            HIVELOG_DEBUG(m_log, "calling RX callback soon"
                ", frame [" << hexdump(frame)
                    << "], error " << err);

            m_stream.get_io_service().post(
                boost::bind(m_rx_callback, err, frame));
        }
        else
        {
            HIVELOG_DEBUG(m_log, "no RX callback, frame"
                " ignored: [" << hexdump(frame)
                    << "], error " << err);
        }
    }

private:

    /// @brief The "send" task.
    /**
    Contains frame and the callback functor.
    */
    class SendTask
    {
    public:

        /// @brief The main constructor.
        /**
        @param[in] callback_ The callback functor.
        @param[in] frame_ The TX frame.
        */
        SendTask(SendFrameCallback callback_, FrameSPtr frame_)
            : callback(callback_), frame(frame_)
        {}

    public:
        SendFrameCallback callback; ///< @brief The callback functor.
        FrameSPtr frame; ///< @brief The TX frame.
    };

    /// @brief The "send" task shared pointer type.
    typedef boost::shared_ptr<SendTask> SendTaskSPtr;

private:

    /// @brief Do the next TX task if any.
    /**
    Starts new TX task if any.
    Does nothing if no pending TX tasks.
    */
    void startNextTxTask()
    {
        HIVELOG_TRACE_BLOCK(m_log, "startNextTxTask()");

        if (!m_tx_in_progress)
        {
            if (!m_tx_tasks.empty())
            {
                HIVELOG_DEBUG_STR(m_log, "sending first frame from the TX queue");

                SendTaskSPtr task = m_tx_tasks.front();
                m_tx_tasks.pop_front();

                asyncWriteAll(task);
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no more frames to send");
        }
        else
            HIVELOG_DEBUG_STR(m_log, "async write in progress, do nothing");
    }


    /// @brief Start write operation.
    /**
    Do NOT call this method again until previous task finished.
    @param[in] task The TX task object.
    */
    void asyncWriteAll(SendTaskSPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncWriteAll()");
        assert(!m_tx_in_progress && "active TX task not finished");

        HIVELOG_DEBUG(m_log, "async write frame ["
            << hexdump(task->frame) << "], "
            << task->frame->size() << " bytes");

        m_tx_in_progress = true;
        boost::asio::async_write(m_stream,
            boost::asio::buffer(task->frame->getContent()),
            boost::bind(&This::onWriteAll, this->shared_from_this(),
                task, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief Write operation finished.
    /**
    @param[in] task The TX task object.
    @param[in] err The error code.
    @param[in] len The number of bytes transfered.
    */
    void onWriteAll(SendTaskSPtr task, boost::system::error_code err, size_t len)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onWriteAll()");
        HIVELOG_TRACE(m_log, "arguments: err="
            << err << ", len=" << len);
        m_tx_in_progress = false;

        if (!err)
        {
            HIVELOG_DEBUG(m_log, len << " bytes have been written");
        }
        else
        {
            if (err == boost::asio::error::operation_aborted)
                HIVELOG_DEBUG(m_log, "write operation cancelled");
            else
                HIVELOG_ERROR(m_log, "write operation error: ["
                    << err << "] " << err.message());
        }

        done(err, task);

        if (!err) // start the pending TX task if any
            startNextTxTask();
    }


    /// @brief Report TX frame to the subscriber.
    /**
    @param[in] err The error code.
    @param[in] task The TX task object.
    */
    void done(boost::system::error_code err, SendTaskSPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "done(tx)");

        HIVELOG_DEBUG(m_log, "calling TX callback soon"
            ", frame [" << hexdump(task->frame)
                << "], error " << err);

        m_stream.get_io_service().post(
            boost::bind(task->callback,
                err, task->frame));
    }

private:

    /// @brief The RX callback.
    RecvFrameCallback m_rx_callback;

    /// @brief The RX buffer.
    boost::asio::streambuf m_rx_buf;

    /// @brief The RX operation "in progress" flag.
    bool m_rx_in_progress;

private:

    /// @brief The list of pending TX tasks.
    std::deque<SendTaskSPtr> m_tx_tasks;

    /// @brief The active TX task.
    bool m_tx_in_progress;

protected:
    StreamT &m_stream; ///< @brief The external stream.
    log::Logger m_log; ///< @brief The logger instance.
};

} // binary namespace

#endif // __SANDBOX_BINARY_HPP_
