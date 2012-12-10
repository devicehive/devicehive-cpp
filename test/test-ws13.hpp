/** @file
@brief The WebSocket unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/ws13.hpp>
#include <iostream>

namespace
{
    using namespace hive;

// send message callback
void on_ws13_send_msg(boost::system::error_code err, ws13::Message::SharedPtr msg)
{
    if (!err)
    {
        std::cout << "\nmsg-send: [" << dump::hex(msg->getData()) << "] \""
            << dump::ascii(msg->getData()) << "\"\n";
    }
    else
        std::cout << "\nmsg-send ERROR: [" << err << "] - " << err.message() << "\n";
}


// recv message callback
void on_ws13_recv_msg(boost::system::error_code err, ws13::Message::SharedPtr msg)
{
    if (!err)
    {
        std::cout << "\nmsg-recv: [" << dump::hex(msg->getData()) << "] \""
            << dump::ascii(msg->getData()) << "\"\n";
    }
    else
        std::cout << "\nmsg-recv ERROR: [" << err << "] - " << err.message() << "\n";
}


// send frame callback
void on_ws13_send_frame(boost::system::error_code err, ws13::Frame::SharedPtr frame)
{
    if (!err)
    {
        std::cout << "\nframe-send: [" << dump::hex(frame->getContent()) << "] \""
            << dump::ascii(frame->getContent()) << "\"\n";
    }
    else
        std::cout << "\nframe-send ERROR: [" << err << "] - " << err.message() << "\n";
}


// recv frame callback
void on_ws13_recv_frame(boost::system::error_code err, ws13::Frame::SharedPtr frame)
{
    if (!err)
    {
        std::cout << "\nframe-recv: [" << dump::hex(frame->getContent()) << "] \""
            << dump::ascii(frame->getContent()) << "\"\n";
    }
    else
        std::cout << "\nframe-recv ERROR: [" << err << "] - " << err.message() << "\n";
}


// websocket connected callback
void on_ws13_connected(boost::system::error_code err, ws13::WebSocket::SharedPtr ws)
{
    if (!err)
    {
        std::cout << "\nconnected\n";

        //ws->asyncSendFrame(ws13::Frame::create(ws13::Frame::Text("Hello from DeviceHive's WebSocket unit (frame)!"), true, ws13::WebSocket::generateNewMask()), on_ws13_send_frame);
        ws->asyncSendMessage(ws13::Message::create("Hello from DeviceHive's WebSocket unit (message)!"), on_ws13_send_msg);//, 16);
    }
    else
    {
        std::cerr << "WebSocket connection failed: ["
            << err << "] - " << err.message() << "\n";
    }
}

template<typename T>
void check_ws13(T const& payload)
{
    std::cout << payload.text << "\n";

    T payload2;
    ws13::Frame::create(payload, false)->getPayload(payload2);
    std::cout << payload2.text << "\n";

    T payload3;
    ws13::Frame::create(payload, true, (rand()<<16) | rand())->getPayload(payload3);
    std::cout << payload3.text << "\n";
}


// test application entry point: print frame examples from 5.7 of RFC6455
void test_ws13_0()
{
    std::cout << "single-frame unmasked text \"Hello\":\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Text("Hello"), false)->getContent())
        << "\n\n";

    std::cout << "single-frame masked text \"Hello\":\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Text("Hello"), true, 0x37fa213d)->getContent())
        << "\n\n";

    std::cout << "fragmented unmasked text message \"Hel\"-\"lo\":\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Text("Hel"), false, 0, false)->getContent()) << "\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Continue("lo"), false)->getContent())
        << "\n\n";

    std::cout << "umasked Ping and masked Pong with \"Hello\" body:\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Ping("Hello"), false)->getContent()) << "\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Pong("Hello"), true, 0x37fa213d)->getContent())
        << "\n\n";

    std::cout << "256 bytes binary message in a single unmasked frame:\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Binary(ws13::OctetString(256, '\xAA')), false)->getContent()).substr(0, 64)
        << "...\n\n";

    std::cout << "64K bytes binary message in a single unmasked frame:\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Binary(ws13::OctetString(64*1024, '\xBB')), false)->getContent()).substr(0, 64)
        << "...\n\n";

    std::cout << "Close message in a single unmasked frame:\n"
        << dump::hex(ws13::Frame::create(ws13::Frame::Close(1111, "Test Error"), false)->getContent())
        << "\n\n";

    check_ws13(ws13::Frame::Text("HelloWS!"));
}


// test application entry point
void test_ws13_1()
{
    log::Logger::root().setLevel(log::LEVEL_INFO);

    boost::asio::io_service ios;

    // client: usually one per application
    http::ClientPtr client = http::Client::create(ios);
    ws13::WebSocket::SharedPtr ws = ws13::WebSocket::create();

    std::cout << "\n\nconnect to the WebSocket server\n";

    ws->connect(http::Url("http://echo.websocket.org/"), client, on_ws13_connected, 20000);
    ws->asyncListenForMessages(on_ws13_recv_msg);
    ws->asyncListenForFrames(on_ws13_recv_frame);

    ios.run();
}

} // local namespace
