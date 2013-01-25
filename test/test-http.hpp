/** @file
@brief The HTTP unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/http.hpp>
#include <iostream>

namespace
{
    using namespace hive;

// assert macro, throws exception
#define MY_ASSERT(cond, msg) \
    if (cond) {} else throw std::runtime_error(msg)

// callback: print the request/response to standard output
void on_http_print(boost::system::error_code err, http::RequestPtr request, http::ResponsePtr response)
{
    std::cout << "\n\n>>> HTTP client response >>>>>>>>>>>>>>>>>\n";
    if (request)
        std::cout << "REQUEST:\n" << *request << "\n";
    if (response)
        std::cout << "RESPONSE:\n" << *response << "\n";
    if (err)
        std::cout << "ERROR: " << err.message() << "\n";
    std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n";
}

void check_base64(String const& data)
{
    const String a = http::base64_encode(data.begin(), data.end());
    const std::vector<UInt8> b = http::base64_decode(a);

    std::cout << "[" << dump::hex(data) << "] => \""
        << a << "\" => [" << dump::hex(b) << "]\n";

    MY_ASSERT(data == String(b.begin(), b.end()),
        "invalid decode(encode), WTF?");
}


// test application entry point: check URL
void test_http0()
{
    http::Url url("http://ecloud.dataart.com/ecapi6?a=1&b=2");
    std::cout << url.toString() << "\n";

    http::Url::Builder urlb(url);
    urlb.appendPath("device/test");
    std::cout << urlb.build().toString() << "\n";

    check_base64("");
    check_base64("\x01");
    check_base64("\x01\x02");
    check_base64("\xde\xad\xff");
    check_base64("\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10");
}


// test application entry point: download file
void test_http1()
{
    hive::log::Logger::root().setLevel(hive::log::LEVEL_DEBUG);

    // client: usually one per application
    boost::asio::io_service ios;
    http::ClientPtr client = http::Client::create(ios);

    // prepare request
    //client->send(http::Request::GET(http::Url("http://httpbin.org/headers")), on_http_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/user-agent")), on_http_print, 10000);
    //client->send(http::Request::GET(http::Url("https://httpbin.org/ip")), on_http_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/redirect/2")), on_http_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/stream/2")), on_http_print, 10000);
    client->send(http::Request::GET(http::Url("http://httpbin.org/delay/5")), on_http_print, 10000);

    ios.run();
}

} // local namespace
