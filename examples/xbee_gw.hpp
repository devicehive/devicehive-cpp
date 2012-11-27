/** @file
@brief The ZigBee gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
//@see @ref page_ex03
#ifndef __EXAMPLES_XBEE_GW_HPP_
#define __EXAMPLES_XBEE_GW_HPP_

#include <DeviceHive/xbee.hpp>
#include "basic_app.hpp"


/// @brief The ZigBee gateway example.
namespace xbee_gw
{
    using namespace hive;


/// @brief The ZigBee gateway application.
class Application:
    public basic_app::Application
{
    typedef basic_app::Application Base; ///< @brief The base class.
protected:

    /// @brief The default constructor.
    Application()
        : m_serialOpenTimer(m_ios)
        , m_serial(m_ios)
        , m_serialBaudrate(0)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Application> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] argc The number of command line arguments.
    @param[in] argv The command line arguments.
    @return The new application instance.
    */
    static SharedPtr create(int argc, const char* argv[])
    {
        SharedPtr pthis(new Application());

        String serialPortName = "COM28";
        UInt32 serialBaudrate = 115200;

        // custom device properties
        for (int i = 1; i < argc; ++i) // skip executable name
        {
            if (boost::algorithm::iequals(argv[i], "--help"))
            {
                std::cout << argv[0] << " [options]";
                std::cout << "\t--serial <serial device name>\n";
                std::cout << "\t--baudrate <serial baudrate>\n";

                throw std::runtime_error("STOP");
            }
            else if (boost::algorithm::iequals(argv[i], "--serial") && i+1 < argc)
                serialPortName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--baudrate") && i+1 < argc)
                serialBaudrate = boost::lexical_cast<UInt32>(argv[++i]);
        }

        pthis->m_xbee = XBeeAPI::create(pthis->m_serial);
        pthis->m_serialPortName = serialPortName;
        pthis->m_serialBaudrate = serialBaudrate;

        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::shared_dynamic_cast<Application>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open serial port.
    */
    virtual void start()
    {
        Base::start();
        asyncOpenSerial(0);
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        //m_xbee->cancelAll();
        m_serialOpenTimer.cancel();
        m_serial.close();
        m_xbee->recv(XBeeAPI::RecvFrameCallback()); // stop listening to release shared pointer
        Base::stop();
    }

private:

    /// @brief Try to open serial device asynchronously.
    /**
    @param[in] wait_sec The number of seconds to wait before open.
    */
    void asyncOpenSerial(long wait_sec)
    {
        HIVELOG_TRACE(m_log, "try to open serial"
            " after " << wait_sec << " seconds");

        m_serialOpenTimer.expires_from_now(boost::posix_time::seconds(wait_sec));
        m_serialOpenTimer.async_wait(boost::bind(&Application::onOpenSerial,
            shared_from_this(), boost::asio::placeholders::error));
    }


    /// @brief Try to open serial device.
    /**
    @return The error code.
    */
    boost::system::error_code openSerial()
    {
        boost::asio::serial_port & port = m_serial;
        boost::system::error_code err;

        port.close(err); // (!) ignore error
        port.open(m_serialPortName, err);
        if (err) return err;

        // set baud rate
        port.set_option(boost::asio::serial_port::baud_rate(m_serialBaudrate), err);
        if (err) return err;

        // set character size
        port.set_option(boost::asio::serial_port::character_size(), err);
        if (err) return err;

        // set flow control
        port.set_option(boost::asio::serial_port::flow_control(), err);
        if (err) return err;

        // set stop bits
        port.set_option(boost::asio::serial_port::stop_bits(), err);
        if (err) return err;

        // set parity
        port.set_option(boost::asio::serial_port::parity(), err);
        if (err) return err;

        return err; // OK
    }


    /// @brief Try to open serial port device callback.
    /**
    This method is called as the "open serial timer" callback.

    @param[in] err The error code.
    */
    void onOpenSerial(boost::system::error_code err)
    {
        if (!err)
        {
            boost::system::error_code serr = openSerial();
            if (!serr)
            {
                HIVELOG_DEBUG(m_log,
                    "got serial device \"" << m_serialPortName
                    << "\" at baudrate: " << m_serialBaudrate);

                asyncListenForGatewayFrames(true);

                sendFrame(xbee::Frame::ATCommandRequest("AP", 1));
                sendFrame(xbee::Frame::ATCommandRequest("SH", 2));
                sendFrame(xbee::Frame::ATCommandRequest("SL", 3));
                sendFrame(xbee::Frame::ZBTransmitRequest("hello from API", 4));
            }
            else
            {
                HIVELOG_DEBUG(m_log, "cannot open serial device \""
                    << m_serialPortName << "\": ["
                    << serr << "] " << serr.message());

                // try to open later
                asyncOpenSerial(10); // TODO: reconnect timeout?
            }
        }
        else if (err == boost::asio::error::operation_aborted)
            HIVELOG_DEBUG_STR(m_log, "open serial device timer cancelled");
        else
            HIVELOG_ERROR(m_log, "open serial device timer error: ["
                << err << "] " << err.message());
    }


    /// @brief Reset the serial device.
    void resetSerial()
    {
        HIVELOG_WARN(m_log, "serial device reset");
        m_serial.close();
        asyncOpenSerial(0);
    }

private:

    /// @brief Send the custom frame.
    /**
    @param[in] payload The custom message payload.
    */
    template<typename PayloadT>
    void sendFrame(PayloadT const& payload)
    {
        m_xbee->send(xbee::Frame::create(payload),
            boost::bind(&Application::onSendFrame,
                shared_from_this(), _1, _2));
    }


    /// @brief The "send frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame sent.
    */
    void onSendFrame(boost::system::error_code err, xbee::Frame::SharedPtr frame)
    {
        if (!err && frame)
        {
            HIVELOG_DEBUG(m_log, "frame successfully sent #"
                << frame->getIntent() << ": ["
                << m_xbee->hexdump(frame) << "], "
                << frame->size() << " bytes:\n"
                << dump(frame));
        }
        else if (err == boost::asio::error::operation_aborted)
        {
            HIVELOG_DEBUG_STR(m_log, "TX operation cancelled");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to send frame: ["
                << err << "] " << err.message());
            resetSerial();
        }
    }

private:

    /// @brief Start/stop listen for RX frames.
    void asyncListenForGatewayFrames(bool enable)
    {
        m_xbee->recv(enable
            ? boost::bind(&Application::onRecvFrame, shared_from_this(), _1, _2)
            : XBeeAPI::RecvFrameCallback());
    }


    /// @brief The "recv frame" callback.
    /**
    @param[in] err The error code.
    @param[in] frame The frame received.
    */
    void onRecvFrame(boost::system::error_code err, xbee::Frame::SharedPtr frame)
    {
        if (!err)
        {
            if (frame)
            {
                HIVELOG_DEBUG(m_log, "frame received #"
                    << frame->getIntent() << ": ["
                    << m_xbee->hexdump(frame) << "], "
                    << frame->size() << " bytes:\n"
                    << dump(frame));
                handleRecvFrame(frame);
            }
            else
                HIVELOG_DEBUG_STR(m_log, "no frame received");
        }
        else if (err == boost::asio::error::operation_aborted)
        {
            HIVELOG_DEBUG_STR(m_log, "RX operation cancelled");
        }
        else
        {
            HIVELOG_ERROR(m_log, "failed to receive frame: ["
                << err << "] " << err.message());
            resetSerial();
        }
    }

private:

    /// @brief Handle the received frame.
    /**
    @param[in] frame The frame.
    */
    void handleRecvFrame(xbee::Frame::SharedPtr frame)
    {
        // TODO: handle the frame
    }

private:

    /// @brief Dump the frame to string.
    /**
    @param[in] frame The frame to dump.
    @return The dump information.
    */
    static String dump(xbee::Frame::SharedPtr frame)
    {
        using xbee::Frame;
        OStringStream oss;

        if (frame)
        switch (frame->getIntent())
        {
            case Frame::ATCOMMAND_REQUEST:
            {
                Frame::ATCommandRequest payload;
                if (frame->getPayload(payload))
                {
                    oss << "ATCOMMAND_REQUEST:"
                        << " frameId=" << int(payload.frameId)
                        << " command=\"" << payload.command << "\"";
                }
                else
                    oss << "ATCOMMAND_REQUEST: bad format";
            } break;

            case Frame::ATCOMMAND_RESPONSE:
            {
                Frame::ATCommandResponse payload;
                if (frame->getPayload(payload))
                {
                    oss << "ATCOMMAND_RESPONSE:"
                        << " frameId=" << int(payload.frameId)
                        << " command=\"" << payload.command << "\""
                        << " status=" << int(payload.status)
                        << " result=[" << dump::hex(payload.result) << "]";
                }
                else
                    oss << "ATCOMMAND_RESPONSE: bad format";
            } break;

            case Frame::ZB_TRANSMIT_REQUEST:
            {
                Frame::ZBTransmitRequest payload;
                if (frame->getPayload(payload))
                {
                    oss << "ZB_TRANSMIT_REQUEST:"
                        << " frameId=" << int(payload.frameId)
                        << " DA64=" << dump::hex(payload.dstAddr64)
                        << " DA16=" << dump::hex(payload.dstAddr16)
                        << " bcastRadius=" << int(payload.bcastRadius)
                        << " options=" << dump::hex(payload.options)
                        << " data=[" << dump::hex(payload.data)
                        << "] (ascii:\"" << payload.data << "\")";
                }
                else
                    oss << "ZB_TRANSMIT_REQUEST: bad format";
            } break;

            case Frame::ZB_TRANSMIT_STATUS:
            {
                Frame::ZBTransmitStatus payload;
                if (frame->getPayload(payload))
                {
                    oss << "ZB_TRANSMIT_STATUS:"
                        << " frameId=" << int(payload.frameId)
                        << " DA16=" << dump::hex(payload.dstAddr16)
                        << " retryCount=" << int(payload.retryCount)
                        << " delivery=" << dump::hex(payload.deliveryStatus)
                        << " discovery=" << dump::hex(payload.discoveryStatus);
                }
                else
                    oss << "ZB_TRANSMIT_STATUS: bad format";
            } break;

            case Frame::ZB_RECEIVE_PACKET:
            {
                Frame::ZBReceivePacket payload;
                if (frame->getPayload(payload))
                {
                    oss << "ZB_RECEIVE_PACKET:"
                        << " SA64=" << dump::hex(payload.srcAddr64)
                        << " SA16=" << dump::hex(payload.srcAddr16)
                        << " options=" << dump::hex(payload.options)
                        << " data=[" << dump::hex(payload.data)
                        << "] (ascii:\"" << payload.data << "\")";
                }
                else
                    oss << "ZB_RECEIVE_PACKET: bad format";
            } break;

            default:
                oss << "unknown frame type: " << frame->getIntent();
                break;
        }

        return oss.str();
    }

private:
    boost::asio::deadline_timer m_serialOpenTimer; ///< @brief Open the serial port device timer.
    boost::asio::serial_port m_serial; ///< @brief The serial port device.
    String m_serialPortName; ///< @brief The serial port name.
    UInt32 m_serialBaudrate; ///< @brief The serial baudrate.

private:
    typedef xbee::API<boost::asio::serial_port> XBeeAPI; ///< @brief The XBee %API type.
    XBeeAPI::SharedPtr m_xbee; ///< @brief The XBee %API.
};

} // xbee_gw namespace


///////////////////////////////////////////////////////////////////////////////
/* @page page_ex03 C++ ZigBee gateway example

!!!UNDER CONSTRUCTION!!!


@example Examples/xbee_gw.hpp
*/

#endif // __EXAMPLES_XBEE_GW_HPP_
