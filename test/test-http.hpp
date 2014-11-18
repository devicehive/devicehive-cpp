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
void on_http_print(http::Client::TaskPtr task)
{
    std::cout << "\n\n>>> HTTP client response >>>>>>>>>>>>>>>>>\n";
    if (task->request)
        std::cout << "REQUEST:\n" << *task->request << "\n";
    if (task->response)
        std::cout << "RESPONSE:\n" << *task->response << "\n";
    if (task->errorCode)
        std::cout << "ERROR: " << task->errorCode.message() << "\n";
    std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n";
}

void check_base64(String const& data)
{
    const String a = misc::base64_encode(data);
    const std::vector<UInt8> b = misc::base64_decode(a);

    std::cout << "[" << dump::hex(data) << "] => \""
        << a << "\" => [" << dump::hex(b) << "]\n";

    MY_ASSERT(data == String(b.begin(), b.end()),
        "invalid decode(encode), WTF?");
}


// test application entry point: check URL
void test_http0()
{
    http::Url url("http://ecloud.dataart.com/ecapi6?a=1&b=2");
    std::cout << url.toStr() << "\n";

    http::Url::Builder urlb(url);
    urlb.appendPath("device/test");
    std::cout << urlb.build().toStr() << "\n";

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

    // prepare and send request
    const http::Url url("http://httpbin.org/delay/5");
    //const http::Url url("http://httpbin.org/headers");
    //const http::Url url("http://httpbin.org/user-agent");
    //const http::Url url("https://httpbin.org/ip");
    //const http::Url url("http://httpbin.org/redirect/2");
    //const http::Url url("http://httpbin.org/stream/2");
    if (http::Client::TaskPtr task = client->send(http::Request::GET(url), 10000))
        task->callWhenDone(boost::bind(on_http_print, task));

    ios.run();
}


// test application entry point: keep-alive connection
void test_http2()
{
    class Test:
        public boost::enable_shared_from_this<Test>
    {
    public:
        Test()
        {
            m_http = http::Client::create(m_ios);
        }

        void run()
        {
            test1();
            m_ios.run();
        }

    private:

        void test1()
        {
            const http::Url url("http://httpbin.org/ip");
            if (http::Client::TaskPtr task = m_http->send(http::Request::GET(url), 10000))
                task->callWhenDone(boost::bind(&Test::on_print1, shared_from_this(), task));
        }

        void test2()
        {
            const http::Url url("http://httpbin.org/delay/5");
            http::RequestPtr req = http::Request::GET(url);
            req->addHeader(http::header::Connection, "close");
            if (http::Client::TaskPtr task = m_http->send(req, 10000))
                task->callWhenDone(boost::bind(&Test::on_print2, shared_from_this(), task));
        }

    private:

        void on_print1(http::Client::TaskPtr task)
        {
            std::cout << "\n\nFirst response:";
            on_http_print(task);
            m_ios.post(boost::bind(&Test::test2, shared_from_this()));
        }

        void on_print2(http::Client::TaskPtr task)
        {
            std::cout << "\n\nSecond response:";
            on_http_print(task);
        }

    private:
        boost::asio::io_service m_ios;
        http::ClientPtr m_http;
    };

    hive::log::Logger::root().setLevel(hive::log::LEVEL_DEBUG);

    boost::shared_ptr<Test> t(new Test());
    t->run();
}

} // local namespace
