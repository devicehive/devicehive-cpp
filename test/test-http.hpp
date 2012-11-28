/** @file
@brief The HTTP unit test.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
*/
#include <hive/http.hpp>
#include <iostream>

namespace
{
    using namespace hive;

// callback: print the request/response to standard output
void on_print(boost::system::error_code err, http::RequestPtr request, http::ResponsePtr response)
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


// test application entry point: check URL
void test_http0()
{
    http::Url url("http://ecloud.dataart.com/ecapi6?a=1&b=2");
    std::cout << url.toString() << "\n";

    http::Url::Builder urlb(url);
    urlb.appendPath("device/test");
    std::cout << urlb.build().toString() << "\n";
}


// test application entry point: download file
void test_http1()
{
    log::Logger::root().setLevel(log::LEVEL_DEBUG);

    // client: usually one per application
    boost::asio::io_service ios;
    http::ClientPtr client = http::Client::create(ios);

    // prepare request
    //client->send(http::Request::GET(http::Url("http://httpbin.org/headers")), on_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/user-agent")), on_print, 10000);
    //client->send(http::Request::GET(http::Url("https://httpbin.org/ip")), on_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/redirect/2")), on_print, 10000);
    //client->send(http::Request::GET(http::Url("http://httpbin.org/stream/2")), on_print, 10000);
    client->send(http::Request::GET(http::Url("http://httpbin.org/delay/5")), on_print, 10000);

    ios.run();
}

} // local namespace
