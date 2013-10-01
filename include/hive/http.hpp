/** @file
@brief The HTTP classes.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>

Some implementation parts is based on urdl library by Christopher M. Kohlhoff.

@see @ref page_hive_http
*/
#ifndef __HIVE_HTTP_HPP_
#define __HIVE_HTTP_HPP_

#include "defs.hpp"
#include "misc.hpp"
#include "log.hpp"

#if !defined(HIVE_PCH)
#   include <boost/enable_shared_from_this.hpp>
#   include <boost/algorithm/string.hpp>
#   include <boost/lexical_cast.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <boost/bind.hpp>
#   include <map>
#endif // HIVE_PCH

#if !defined(HIVE_DISABLE_SSL)
#   include <boost/asio/ssl.hpp>
#endif // HIVE_DISABLE_SSL


namespace hive
{
    /// @brief The HTTP module.
    /**
    This namespace contains classes and functions related to HTTP communication.
    */
    namespace http
    {

/// @brief The URL.
/**
This class parses the HTTP URL and provides access to the URL's components.

The following format of URL is expected:
    http://user:password@host:port/path/file?query#fragment


Now supports `http`, `https`, `ftp`, `ws` and `wss` protocols.

The URL encoding/decoding functions are provided as static methods.

There also equality operators == and != are available.

Please use Builder class to build advanced URLs.
*/
class Url
{
public:
    class Builder; // will be defined later

public:

    /// @brief The default constructor.
    /**
    All components are empty.
    */
    Url()
    {}


    /// @brief The main constructor (C-string).
    /**
    @param[in] url The URL string to parse.
    @throws std::invalid_argument If the URL string is invalid.
    */
    /*explicit*/ Url(const char* url)
    {
        parse(url);
    }


    /// @brief The main constructor.
    /**
    @param[in] url The URL string to parse.
    @throws std::invalid_argument If the URL string is invalid.
    */
    /*explicit*/ Url(String const& url)
    {
        parse(url.c_str());
    }


    /// @brief The auxiliary constructor.
    /**
    This constructor is used mainly by Builder.

    @param[in] proto The protocol.
    @param[in] user The user information.
    @param[in] host The host.
    @param[in] port The port number.
    @param[in] path The path.
    @param[in] query The optional query.
    @param[in] fragment The fragment.
    */
    Url(String const& proto, String const& user,
        String const& host, String const& port, String const& path,
        String const& query, String const& fragment)
        : m_proto(proto)
        , m_user(user)
        , m_host(host)
        , m_port(port)
        , m_path(path)
        , m_query(query)
        , m_fragment(fragment)
    {}


/// @name Access to URL's components
/// @{
public:

    /// @brief Get the protocol component.
    /**
    @return The protocol component.
    */
    String const& getProtocol() const
    {
        return m_proto;
    }


    /// @brief Get the user information component.
    /**
    @return The user information component.
    */
    String const& getUserInfo() const
    {
        return m_user;
    }


    /// @brief Get the host component.
    /**
    @return The host component.
    */
    String const& getHost() const
    {
        return m_host;
    }


    /// @brief Get the port component.
    /**
    @return The port component.
    */
    String const& getPort() const
    {
        return m_port;
    }


    /// @brief Get the integer port number.
    /**
    Return default port number depend on protocol:

    | protocol | port
    |----------|------
    |     http | 80
    |    https | 443
    |      ftp | 21
    |       ws | 80
    |      wss | 443

    @return The port number as integer.
    */
    UInt32 getPortNumber() const
    {
        if (m_port.empty())
        {
            if (boost::iequals(m_proto, "http"))
                return 80;
            else if (boost::iequals(m_proto, "https"))
                return 443;
            else if (boost::iequals(m_proto, "ftp"))
                return 21;
            else if (boost::iequals(m_proto, "ws"))
                return 80;
            else if (boost::iequals(m_proto, "wss"))
                return 443;
            else
                return 0; // unknown
        }
        else
        {
            UInt32 port = 0;

            IStringStream ss(m_port);
            ss >> port; // ignore parsing errors

            return port;
        }
    }


    /// @brief Get the path component.
    /**
    @return The path component.
    */
    String const& getPath() const
    {
        return m_path;
    }


    /// @brief Get the query component.
    /**
    @return The query component.
    */
    String const& getQuery() const
    {
        return m_query;
    }


    /// @brief Get the fragment component.
    /**
    @return The fragment component.
    */
    String const& getFragment() const
    {
        return m_fragment;
    }
/// @}

public:

    /// @brief The components of the URL.
    enum Components
    {
        PROTOCOL  = 0x01, ///< @hideinitializer @brief The protocol.
        USER_INFO = 0x02, ///< @hideinitializer @brief The user information.
        HOST      = 0x08, ///< @hideinitializer @brief The host.
        PORT      = 0x10, ///< @hideinitializer @brief The port.
        PATH      = 0x20, ///< @hideinitializer @brief The path.
        QUERY     = 0x40, ///< @hideinitializer @brief The query.
        FRAGMENT  = 0x80, ///< @hideinitializer @brief The fragment.

        /// @brief All available components.
        ALL = PROTOCOL|USER_INFO
            |HOST|PORT|PATH
            |QUERY|FRAGMENT
    };


    /// @brief Convert to a string.
    /**
    @param[in] components A bitmask specifying which components
        of the URL should be included into the output string.
    @see Components enumeration for possible values.
    @return A string representation of the URL.
    */
    String toStr(int components = ALL) const
    {
        OStringStream res;

        // protocol
        if ((components&PROTOCOL) && !m_proto.empty())
            res << m_proto << "://";

        // user name & password
        if ((components&USER_INFO) && !m_user.empty())
            misc::url_encode(res, m_user) << "@";

        // host
        if ((components&HOST) && !m_host.empty())
            res << m_host;

        // port
        if ((components&PORT) && !m_port.empty())
            res << ":" << m_port;

        // path
        if ((components&PATH) && !m_path.empty())
            res << m_path; //misc::url_encode(res, m_path);

        // query
        if ((components&QUERY) && !m_query.empty())
            res << "?" << m_query; //misc::url_encode(res << "?", m_query);

        // fragment
        if ((components&FRAGMENT) && !m_fragment.empty())
            res << "#" << m_fragment; // misc::url_encode(res << "#", m_fragment);

        return res.str();
    }

private:

    /// @brief Parse a string representation of an URL.
    /**
    @param url The URL string to parse.
    @throws std::invalid_argument If URL string is invalid.
    */
    void parse(const char* url)
    {
        // protocol
        if (const char* p = strstr(url, "://"))
        {
            const size_t len = (p - url);

            m_proto.assign(url, p);
            //m_proto = misc::url_decode(m_proto);
            boost::algorithm::to_lower(m_proto);

            url += len + 3;
        }
        else
            throw std::invalid_argument("no protocol scheme");

        { // user information
            const size_t len = strcspn(url, "@:[/?#");
            if (url[len] == '@')
            {
                m_user.assign(url, url + len);
                m_user = misc::url_decode(m_user);
                url += len + 1;
            }
            else if (url[len] == ':')
            {
                const size_t len2 = strcspn(url+len, "@/?#");
                if (url[len+len2] == '@')
                {
                    m_user.assign(url, url + len+len2);
                    m_user = misc::url_decode(m_user);
                    url += len+len2 + 1;
                }
            }
        }

        // host
        if (url[0] == '[')
        {
            throw std::invalid_argument("IPv6 address not supported");
        }
        else
        {
            const size_t len = std::strcspn(url, ":/?#");
            m_host.assign(url, url + len);
            //m_host = misc::url_decode(m_host);
            url += len;
        }

        // port
        if (url[0] == ':')
        {
            const size_t len = std::strcspn(++url, "/?#");
            if (!len)
                throw std::invalid_argument("no port number provided");

            m_port.assign(url, url + len);
            for (size_t i = 0; i < len; ++i)
            {
                if (!misc::is_digit(m_port[i]))
                    throw std::invalid_argument("invalid port number");
            }

            url += len;
        }

        // path
        if (url[0] == '/')
        {
            const size_t len = std::strcspn(url, "?#");
            m_path.assign(url, url + len);
            m_path = misc::url_decode(m_path);
            url += len;
        }
        else
            m_path = "/";

        // query
        if (url[0] == '?')
        {
            const size_t len = std::strcspn(++url, "#");
            m_query.assign(url, url + len);
            m_query = misc::url_decode(m_query);
            url += len;
        }

        // fragment
        if (url[0] == '#')
        {
            m_fragment.assign(++url);
            m_fragment = misc::url_decode(m_fragment);
        }
    }

private:
    String m_proto; ///< @brief The protocol.
    String m_user;  ///< @brief The user information.
    String m_host;  ///< @brief The host.
    String m_port;  ///< @brief The port number.
    String m_path;  ///< @brief The path.

    String m_query; ///< @brief The optional query.
    String m_fragment; ///< @brief The fragment.
};


/// @brief Are two URLs equal?
/**
@relates Url
@param[in] a The first URL.
@param[in] b The second URL.
@return `true` if two URLs are equal.
*/
inline bool operator==(Url const& a, Url const& b)
{
    return a.getProtocol() == b.getProtocol()
        && a.getUserInfo() == b.getUserInfo()
        && a.getHost()     == b.getHost()
        && a.getPort()     == b.getPort()
        && a.getPath()     == b.getPath()
        && a.getQuery()    == b.getQuery()
        && a.getFragment() == b.getFragment();
}


/// @brief Are two URLs different?
/**
@relates Url
@param[in] a The first URL.
@param[in] b The second URL.
@return `true` if two URLs are different.
*/
inline bool operator!=(Url const& a, Url const& b)
{
    return !(a == b);
}


/// @brief The URL builder.
/**
This class helps to build an Url from components.
*/
class Url::Builder
{
public:

    /// @brief The default constructor.
    Builder()
    {}


    /// @brief The main constructor.
    /**
    @param[in] url The base URL.
    */
    explicit Builder(Url const& url)
        : m_proto(url.getProtocol())
        , m_user(url.getUserInfo())
        , m_host(url.getHost())
        , m_port(url.getPort())
        , m_fragment(url.getFragment())
    {
        parsePath(url.getPath());
        parseQuery(url.getQuery());
    }

public:

    /// @brief Build the URL.
    /**
    @return The built URL.
    */
    Url build() const
    {
        return Url(m_proto, m_user,
            m_host, m_port, formatPath(),
            formatQuery(), m_fragment);
    }

/// @name Protocol, user, host, port and fragment
/// @{
public:

    /// @brief Set new protocol.
    /**
    @param[in] proto The new protocol.
    @return Self reference.
    */
    Builder& setProtocol(String const& proto)
    {
        m_proto = proto;
        return *this;
    }


    /// @brief Set user name and password.
    /**
    @param[in] userName The user name.
    @param[in] password The optional password.
    @return Self reference.
    */
    Builder& setUserInfo(String const& userName, String const& password = String())
    {
        m_user = password.empty() ? userName
               : userName + ":" + password;
        return *this;
    }


    /// @brief Set host.
    /**
    @param[in] host The new host.
    @return Self reference.
    */
    Builder& setHost(String const& host)
    {
        m_host = host;
        return *this;
    }


    /// @brief Set port.
    /**
    @param[in] port The new port.
    @return Self reference.
    */
    Builder& setPort(String const& port)
    {
        m_port = port;
        return *this;
    }


    /// @brief Set fragment.
    /**
    @param[in] fragment The new fragment.
    @return Self reference.
    */
    Builder& setFragment(String const& fragment)
    {
        m_fragment = fragment;
        return *this;
    }
/// @}

/// @name Path components
/// @{
public:

    /// @brief Append new path components.
    /**
    @param[in] path The path components to add.
    @return Self reference.
    */
    Builder& appendPath(String const& path)
    {
        parsePath(path);
        return *this;
    }


    /// @brief Remove all path components.
    /**
    @return Self reference.
    */
    Builder& clearPath()
    {
        m_path.clear();
        return *this;
    }
/// @}


/// @name Query components
/// @{
public:

    /// @brief Append new query components.
    /**
    @param[in] query The query components to add.
    @return Self reference.
    */
    Builder& appendQuery(String const& query)
    {
        parseQuery(query);
        return *this;
    }


    /// @brief Remove all query components.
    /**
    @return Self reference.
    */
    Builder& clearQuery()
    {
        m_query.clear();
        return *this;
    }

/// @}

private:

    /// @brief Parse all path components.
    /**
    @param[in] path The path components.
    */
    void parsePath(String const& path)
    {
        IStringStream iss(path);
        String comp;

        while (std::getline(iss, comp, '/'))
        {
            if (!comp.empty())
                m_path.push_back(comp);
        }
    }


    /// @brief Format the path components into the string.
    /**
    @return The path components.
    */
    String formatPath() const
    {
        const size_t N = m_path.size();
        if (0 < N)
        {
            OStringStream oss;
            for (size_t i = 0; i < N; ++i)
                oss << '/' << m_path[i];
            return oss.str();
        }
        else
            return "/";
    }

private:

    /// @brief Parse all query components.
    /**
    @param[in] query The query components.
    */
    void parseQuery(String const& query)
    {
        IStringStream iss(query);
        String comp;

        while (std::getline(iss, comp, '&'))
        {
            if (!comp.empty())
                m_query.push_back(comp);
        }
    }


    /// @brief Format the query components into the string.
    /**
    @return The query components.
    */
    String formatQuery() const
    {
        OStringStream oss;

        const size_t N = m_query.size();
        for (size_t i = 0; i < N; ++i)
        {
            if (i) oss << '&';
            oss << m_query[i];
        }

        return oss.str();
    }

private:
    String m_proto; ///< @brief The protocol.
    String m_user;  ///< @brief The user information.
    String m_host;  ///< @brief The host.
    String m_port;  ///< @brief The port number.
    std::vector<String> m_path;  ///< @brief The path.
    std::vector<String> m_query; ///< @brief The optional query.
    String m_fragment; ///< @brief The fragment.
};


        /// @brief The HTTP headers.
        /**
        This namespace contains definition of common HTTP headers.
        */
        namespace header
        {

const char Host[]               = "Host";               ///< @hideinitializer @brief The "Host" header name.
const char Allow[]              = "Allow";              ///< @hideinitializer @brief The "Allow" header name.
const char Accept[]             = "Accept";             ///< @hideinitializer @brief The "Accept" header name.
const char Connection[]         = "Connection";         ///< @hideinitializer @brief The "Connection" header name.
const char Content_Encoding[]   = "Content-Encoding";   ///< @hideinitializer @brief The "Content-Encoding" header name.
const char Content_Length[]     = "Content-Length";     ///< @hideinitializer @brief The "Content-Length" header name.
const char Content_Type[]       = "Content-Type";       ///< @hideinitializer @brief The "Content-Type" header name.
const char Expires[]            = "Expires";            ///< @hideinitializer @brief The "Expires" header name.
const char Last_Modified[]      = "Last-Modified";      ///< @hideinitializer @brief The "Last-Modified" header name.
const char User_Agent[]         = "User-Agent";         ///< @hideinitializer @brief The "User-Agent" header name.
const char Location[]           = "Location";           ///< @hideinitializer @brief The "Location" header name.
const char Upgrade[]            = "Upgrade";            ///< @hideinitializer @brief The "Upgrade" header name.
const char Authorization[]      = "Authorization";      ///< @hideinitializer @brief The "Authorization" header name.
const char SetCookie[]          = "Set-Cookie";         ///< @hideinitializer @brief The "Set-Cookie" header name.
const char Cookie[]             = "Cookie";             ///< @hideinitializer @brief The "Cookie" header name.

        // TODO: add more headers here

        } // header namespace


        // helpers
        namespace impl
        {

/// @hideinitializer @brief The new line string.
const char CRLF[] = "\r\n";

/// @hideinitializer @brief The empty line and new line string.
const char CRLFx2[] = "\r\n\r\n";

        } // helpers


/// @brief The HTTP message.
/**
This is base class for HTTP requests and responses.
Contains HTTP version, list of HTTP headers, and the body content.
The HTTP version may be changed using setVersion() method.
The body content stored as string using setContent() method.

All headers are stored in map:
    - headers are case insensitive
    - headers are sorted by name

@see @ref header namespace
*/
class Message
{
protected:

    /// @brief The default constructor.
    /**
    The HTTP 1.1 version is used by default.
    */
    Message()
        : m_versionMajor(1)
        , m_versionMinor(1)
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Message()
    {}


/// @name The HTTP version
/// @{
public:

    /// @brief Set the HTTP version.
    /**
    @param[in] major The major HTTP version.
    @param[in] minor The minor HTTP version.
    @return Self reference.
    */
    Message& setVersion(UInt32 major, UInt32 minor)
    {
        m_versionMajor = major;
        m_versionMinor = minor;
        return *this;
    }


    /// @brief Get the major HTTP version.
    UInt32 getVersionMajor() const
    {
        return m_versionMajor;
    }


    /// @brief Get the minor HTTP version.
    UInt32 getVersionMinor() const
    {
        return m_versionMinor;
    }
/// @}


/// @name HTTP headers
/// @{
private:

    /// @brief The less functor.
    /**
    Compares string no case.
    */
    struct iLess
        : public std::binary_function<String, String, bool>
    {
        /// @brief Compare two strings.
        /**
        @param[in] a The first string.
        @param[in] b The second string.
        @return `true` if first string is less than second.
        */
        bool operator()(String const& a, String const& b) const
        {
            return boost::ilexicographical_compare(a, b);
        }
    };

    /// @brief The header container type.
    /**
    Uses case insensitive comparison for header names.
    */
    typedef std::multimap<String, String, iLess> HeaderMap;

public:

    /// @brief The header iterator type.
    typedef HeaderMap::const_iterator HeaderIterator;


    /// @brief Get the begin of headers.
    /**
    @return The begin iterator.
    */
    HeaderIterator headersBegin() const
    {
        return m_headers.begin();
    }


    /// @brief Get the end of headers.
    /**
    @return The end iterator.
    */
    HeaderIterator headersEnd() const
    {
        return m_headers.end();
    }


    /// @brief Get first header.
    /**
    @param[in] name The header name.
    @return The header value or empty string if header doesn't exist.
    @see @ref header namespace for possible names
    */
    String getHeader(String const& name) const
    {
        HeaderMap::const_iterator found = m_headers.find(name);
        return (found != m_headers.end())
            ? found->second : String();
    }


    /// @brief Check if header exists.
    /**
    @param[in] name The header name.
    @return `true` if message contains header.
    @see @ref header namespace for possible names
    */
    bool hasHeader(String const& name) const
    {
        HeaderMap::const_iterator found = m_headers.find(name);
        return (found != m_headers.end());
    }


    /// @brief Add header.
    /**
    @param[in] name The header name.
    @param[in] value The header value.
    @return Self reference.
    @see @ref header namespace for possible names
    */
    Message& addHeader(String const& name, String const& value)
    {
        m_headers.insert(std::make_pair(name, value));
        return *this;
    }


    /// @brief Remove header.
    /**
    @param[in] name The header name.
    @return Self reference.
    @see @ref header namespace for possible names
    */
    Message& removeHeader(String const& name)
    {
        m_headers.erase(name);
        return *this;
    }


    /// @brief Write all headers to the output stream.
    /**
    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& writeAllHeaders(OStream & os) const
    {
        HeaderMap::const_iterator const ie = m_headers.end();
        HeaderMap::const_iterator i = m_headers.begin();

        for (; i != ie; ++i)
        {
            os << i->first << ": "
                << i->second
                << impl::CRLF;
        }

        return os;
    }

/// @name Body content
/// @{
public:

    /// @brief Set content string.
    /**
    @param[in] content The custom content.
    @return Self reference.
    */
    Message& setContent(String const& content)
    {
        m_content = content;
        return *this;
    }


    /// @brief Get content string.
    String const& getContent() const
    {
        return m_content;
    }


    /// @brief Write content to the output stream.
    /**
    The `Content-Length` header will be added automatically if content isn't empty.

    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& writeContent(OStream & os) const
    {
        if (!m_content.empty())
        {
            // add "Content-Length" header
            if (!hasHeader(header::Content_Length))
            {
                os << header::Content_Length << ": "
                    << m_content.size() << impl::CRLF;
            }
            os << impl::CRLF;
            os.write(m_content.data(),
                m_content.size());
        }
        else
            os << impl::CRLF;

        return os;
    }
/// @}

private:
    UInt32 m_versionMajor; ///< @brief The major HTTP version.
    UInt32 m_versionMinor; ///< @brief The minor HTTP version.
    HeaderMap m_headers; ///< @brief The headers.
    String m_content; ///< @brief The custom content.
};


/// @brief The HTTP request.
/**
Contains request method and the URL along with the HTTP version, list of HTTP headers
and request body content derived from Message base class.

This class is designed to be used as dynamic object created with `new` operator.
There are a lot of factory methods:
    - create() creates custom request
    - GET() creates *GET* request
    - PUT() creates *PUT* request
    - POST() creates *POST* request

The request HTTP method and the URL cannot be changed after creation.
*/
class Request:
    public Message
{
protected:

    /// @brief The main constructor.
    /**
    @param[in] method The HTTP method.
    @param[in] url The URL.
    */
    Request(String const& method, Url const& url)
        : m_method(method), m_url(url)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Request> SharedPtr;


    /// @brief The factory method.
    /**
    @param[in] method The HTTP method.
    @param[in] url The URL.
    @return The new request instance.
    */
    static SharedPtr create(String const& method, Url const& url)
    {
        return SharedPtr(new Request(method, url));
    }


    /// @brief Create GET request.
    /**
    @param[in] url The URL.
    @return The new GET request instance.
    */
    static SharedPtr GET(Url const& url)
    {
        return create("GET", url);
    }


    /// @brief Create PUT request.
    /**
    @param[in] url The URL.
    @return The new PUT request instance.
    */
    static SharedPtr PUT(Url const& url)
    {
        return create("PUT", url);
    }


    /// @brief Create PUT request with content.
    /**
    @param[in] url The URL.
    @param[in] contentType The optional content type.
    @param[in] content The optional content.
    @return The new PUT request instance.
    */
    static SharedPtr PUT(Url const& url, String const& contentType, String const& content)
    {
        SharedPtr req = PUT(url);
        req->addHeader(header::Content_Type, contentType);
        req->setContent(content);
        return req;
    }


    /// @brief Create POST request.
    /**
    @param[in] url The URL.
    @return The new POST request instance.
    */
    static SharedPtr POST(Url const& url)
    {
        return create("POST", url);
    }


    /// @brief Create POST request with content.
    /**
    @param[in] url The URL.
    @param[in] contentType The optional content type.
    @param[in] content The optional content.
    @return The new POST request instance.
    */
    static SharedPtr POST(Url const& url, String const& contentType, String const& content)
    {
        SharedPtr req = POST(url);
        req->addHeader(header::Content_Type, contentType);
        req->setContent(content);
        return req;
    }

public:

    /// @brief Get the HTTP method.
    /**
    @return The HTTP method.
    */
    String const& getMethod() const
    {
        return m_method;
    }


    /// @brief Get the URL.
    Url const& getUrl() const
    {
        return m_url;
    }

public:

    /// @brief Write the first line.
    /**
    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& writeFirstLine(OStream & os) const
    {
        return os << getMethod() << " "
            << getUrl().toStr(Url::PATH|Url::QUERY)
            << " HTTP/" << getVersionMajor() << "."
            << getVersionMinor() << impl::CRLF;
    }


    /// @brief Write the whole request.
    /**
    This method writes the first line, HTTP headers and content if present.

    The `Host` header will be added automatically if it's not provided.

    The `Content-Length` header will be added automatically if content isn't empty.

    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& write(OStream & os) const
    {
        writeFirstLine(os);
        writeAllHeaders(os);

        if (!hasHeader(header::Host))
        {
            os << header::Host << ": "
                << getUrl().toStr(Url::HOST|Url::PORT)
                << impl::CRLF;
        }

        writeContent(os);
        return os;
    }

private:
    String m_method; ///< @brief The HTTP method.
    Url m_url;       ///< @brief The URL.
};


/// @brief The HTTP request shared pointer type.
typedef Request::SharedPtr RequestPtr;


/// @brief Write the whole request to the output stream.
/** @relates Request
@param[in,out] os The output stream.
@param[in] req The HTTP request.
@return The output stream.
@see Request::write()
*/
inline OStream& operator<<(OStream &os, Request const& req)
{
    return req.write(os);
}


        /// @brief The HTTP response status codes.
        /**
        This namespace contains definition of common status codes from HTTP/1.0 and HTTP/1.1.
        */
        namespace status
        {

/// @brief Internal (0xx) codes.
enum Internal
{
    UNKNOWN = 0 ///< @hideinitializer @brief Unknown status code.
};


/// @brief Informational (1xx) codes.
enum Informational
{
    CONTINUE = 100 ///< @hideinitializer @brief 100
};


/// @brief Success (2xx) codes.
enum Success
{
    OK         = 200, ///< @hideinitializer @brief 200
    CREATED    = 201, ///< @hideinitializer @brief 201
    ACCEPTED   = 202, ///< @hideinitializer @brief 202
    NO_CONTENT = 204  ///< @hideinitializer @brief 204
};


/// @brief Redirection (3xx) codes.
enum Redirection
{
    MOVED_PERMANENTLY = 301, ///< @hideinitializer @brief 301
    MOVED_TEMPORARILY = 302, ///< @hideinitializer @brief 302
    NOT_MODIFIED      = 304  ///< @hideinitializer @brief 304
};


/// @brief Client Error (4xx) codes.
enum ClientError
{
    BAD_REQUEST  = 400, ///< @hideinitializer @brief 400
    UNAUTHORIZED = 401, ///< @hideinitializer @brief 401
    FORBIDDEN    = 403, ///< @hideinitializer @brief 403
    NOT_FOUND    = 404  ///< @hideinitializer @brief 404
};


/// @brief Server Error (5xx) codes.
enum ServerError
{
    INTERNAL_SERVER_ERROR = 500, ///< @hideinitializer @brief 500
    NOT_IMPLEMENTED       = 501, ///< @hideinitializer @brief 501
    BAD_GATEWAY           = 502, ///< @hideinitializer @brief 502
    SERVICE_UNAVAILABLE   = 503, ///< @hideinitializer @brief 503
    GATEWAY_TIMEOUT       = 504  ///< @hideinitializer @brief 504
};

        } // status namespace


/// @brief The HTTP response.
/**
Contains status code and reason phrase along with the HTTP version, list of HTTP headers
and request body content derived from Message base class.

This class is designed to be used as dynamic object created with `new` operator.
There is factory method create() which creates empty HTTP response.
But you probably don't have to create responses manually.
They are created by the Client automatically.
*/
class Response:
    public Message
{
protected:

    /// @brief The default constructor.
    /**
    @param[in] status The status code.
    @param[in] reason The status phrase.
    */
    Response(int status, String const& reason)
        : m_statusCode(status),
          m_statusPhrase(reason)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Response> SharedPtr;


    /// @brief The main factory method.
    /**
    @param[in] status The status code.
    @param[in] reason The status phrase.
    @return The new response.
    */
    static SharedPtr create(int status = status::UNKNOWN, String const& reason = String())
    {
        return SharedPtr(new Response(status, reason));
    }

/// @name Response status
/// @{
public:

    /// @brief Set the status code.
    /**
    @param[in] status The status code.
    @return Self reference.
    */
    Response& setStatusCode(int status)
    {
        m_statusCode = status;
        return *this;
    }


    /// @brief Get the status code.
    /**
    @return The status code.
    */
    int getStatusCode() const
    {
        return m_statusCode;
    }


    /// @brief Set the status phrase.
    /**
    @param[in] reason The status phrase.
    @return Self reference.
    */
    Response& setStatusPhrase(String const& reason)
    {
        m_statusPhrase = reason;
        return *this;
    }


    /// @brief Get the status phrase.
    /**
    @return The status phrase.
    */
    String const& getStatusPhrase() const
    {
        return m_statusPhrase;
    }


    /// @brief Is status code successful?
    /**
    @return `true` if status code is in range [200..299].
    */
    bool isStatusSuccessful() const
    {
        return 200 <= m_statusCode && m_statusCode < 300;
    }
/// @}

public:

    /// @brief Write status line.
    /**
    This method writes the status line to the output stream.

    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& writeFirstLine(OStream & os) const
    {
        return os << "HTTP/" << getVersionMajor() << "."
            << getVersionMinor() << " " << getStatusCode() << " "
            << getStatusPhrase() << impl::CRLF;
    }


    /// @brief Write the whole response.
    /**
    This method writes the first line, HTTP headers and content if present.

    The `Content-Length` header will be added automatically if content isn't empty.

    @param[in,out] os The output stream.
    @return The output stream.
    */
    OStream& write(OStream & os) const
    {
        writeFirstLine(os);
        writeAllHeaders(os);
        writeContent(os);
        return os;
    }

private:
    int m_statusCode; ///< @brief The status code.
    String m_statusPhrase; ///< @brief The status phrase.
};


/// @brief The HTTP response shared pointer type.
typedef Response::SharedPtr ResponsePtr;


/// @brief Write the whole response to the output stream.
/** @relates Response
@param[in,out] os The output stream.
@param[in] res The HTTP response.
@return The output stream.
@see Response::write()
*/
inline OStream& operator<<(OStream &os, Response const& res)
{
    return res.write(os);
}


/// @brief The base connection.
/**
This class represents one connection to the server.

Contains buffer for send/recv operations.

- Simple for HTTP connections
- Secure for HTTPS connections

Connections are managed by the HTTP client class internally.
*/
class Connection:
    private NonCopyable
{
public:
    typedef boost::asio::io_service IOService; ///< @brief The IO service type.
    typedef boost::system::error_code ErrorCode; ///< @brief The error code type.
    typedef boost::asio::ip::tcp::resolver Resolver; ///< @brief The resolver type.
    typedef boost::asio::ip::tcp::endpoint Endpoint; ///< @brief The endpoint type.
    typedef boost::asio::streambuf StreamBuf; ///< @brief The stream buffer type.
    typedef boost::asio::mutable_buffers_1 MutableBuffers; ///< @brief The mutable buffers.
    typedef boost::asio::const_buffers_1 ConstBuffers; ///< @brief The constant buffers.

protected:

    /// @brief The default constructor.
    Connection()
    {}

public:

    /// @brief The trivial destructor.
    virtual ~Connection()
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Connection> SharedPtr;

    // real connections
    class Simple;
    class Secure;

public:

    /// @brief Get the stream buffer.
    /**
    @return The stream buffer.
    */
    StreamBuf& getBuffer()
    {
        return m_buffer;
    }

public:

    /// @brief Get the IO service.
    /**
    @return The corresponding IO service.
    */
    virtual IOService& get_io_service() = 0;


    /// @brief Get the remote endpoint.
    /**
    @return The remote endpoint.
    */
    virtual Endpoint remote_endpoint() const = 0;


    /// @brief Cancel any asynchronous operations.
    virtual void cancel() = 0;


    /// @brief Close the connection.
    /**
    Cancels all asynchronous operations.
    */
    virtual void close() = 0;

public:

    /// @brief The "connect" operation callback type.
    typedef boost::function1<void, ErrorCode> ConnectCallback;


    /// @brief Start asynchronous "connect" operation.
    /**
    @param[in] epi The endpoint iterator.
    @param[in] callback The callback functor.
    */
    virtual void async_connect(Resolver::iterator epi, ConnectCallback callback) = 0;

public:

    /// @brief The "handshake" operation callback type.
    typedef boost::function1<void, ErrorCode> HandshakeCallback;


    /// @brief Start asynchronous "handshake" operation.
    /**
    @param[in] type The handshake type.
    @param[in] callback The callback functor.
    */
    virtual void async_handshake(int type, HandshakeCallback callback) = 0;

public:

    /// @brief The "write" operation callback.
    typedef boost::function2<void, ErrorCode, size_t> WriteCallback;


    /// @brief Start asynchronous "write" operation.
    /**
    @param[in] bufs The buffers to send.
    @param[in] callback The callback functor.
    */
    virtual void async_write_some(ConstBuffers const& bufs, WriteCallback callback) = 0;

public:

    /// @brief The "read" operation callback.
    typedef boost::function2<void, ErrorCode, size_t> ReadCallback;


    /// @brief Start asynchronous "read some" operation.
    /**
    @param[in] bufs The buffers to read to.
    @param[in] callback The callback functor.
    */
    virtual void async_read_some(MutableBuffers const& bufs, ReadCallback callback) = 0;

private:

    /// @brief The stream buffer.
    /**
    This buffer may be used for read/write operations.
    */
    StreamBuf m_buffer;
};

/// @brief The HTTP connection shared pointer type.
typedef Connection::SharedPtr ConnectionPtr;


/// @brief The simple HTTP connection.
/**
You can create instance using create() factory method.
*/
class Connection::Simple:
    public Connection
{
    typedef Connection Base; ///< @brief The base type.
public:
    typedef boost::asio::ip::tcp::socket TcpSocket; ///< @brief The TCP socket type.

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    */
    explicit Simple(IOService & ios)
        : m_socket(ios)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Simple> SharedPtr;


    /// @brief The main factory method.
    /**
    @param[in] ios The IO service.
    @return The new HTTP connection instance.
    */
    static SharedPtr create(IOService & ios)
    {
        return SharedPtr(new Simple(ios));
    }

public:

    /// @brief Get the socket stream.
    /**
    @return The socket.
    */
    TcpSocket& getStream()
    {
        return m_socket;
    }

public: // Connection

    /// @copydoc Connection::get_io_service()
    virtual IOService& get_io_service()
    {
        return m_socket.get_io_service();
    }


    /// @copydoc Connection::remote_endpoint()
    virtual Endpoint remote_endpoint() const
    {
        return m_socket.remote_endpoint();
    }


    /// @copydoc Connection::cancel()
    virtual void cancel()
    {
        ErrorCode terr;

        m_socket.cancel(terr);
        // ignore error code?
    }


    /// @copydoc Connection::close()
    virtual void close()
    {
        ErrorCode terr;

        m_socket.shutdown(TcpSocket::shutdown_both, terr);
        // ignore error code?

        m_socket.close(terr);
        // ignore error code?
    }


    /// @copydoc Connection::async_connect()
    virtual void async_connect(Resolver::iterator epi, ConnectCallback callback)
    {
        // attempt a connection to each endpoint in the list
        boost::asio::async_connect(
            m_socket, epi, boost::bind(callback,
                boost::asio::placeholders::error));
    }


    /// @copydoc Connection::async_handshake()
    /**
    Does nothing for simple HTTP connections.
    */
    virtual void async_handshake(int type, HandshakeCallback callback)
    {
        // handshake is always successful, just call the callback
        get_io_service().post(boost::bind(callback, ErrorCode()));
    }


    /// @copydoc Connection::async_write_some()
    virtual void async_write_some(ConstBuffers const& bufs, WriteCallback callback)
    {
        m_socket.async_write_some(bufs,
            boost::bind(callback, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @copydoc Connection::async_read_some()
    virtual void async_read_some(MutableBuffers const& bufs, ReadCallback callback)
    {
        m_socket.async_read_some(bufs,
            boost::bind(callback, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:
    TcpSocket m_socket; ///< @brief The socket.
};


#if !defined(HIVE_DISABLE_SSL)

/// @brief The secure HTTPS connection.
/**
You can create instance using create() factory method.
*/
class Connection::Secure:
    public Connection
{
    typedef Connection Base; ///< @brief The base type.
public:
    typedef boost::asio::ip::tcp::socket TcpSocket; ///< @brief The TCP socket type.
    typedef boost::asio::ssl::stream<TcpSocket> SslStream; ///< @brief The SSL stream type.
    typedef boost::asio::ssl::context SslContext; ///< @brief The SSL context type.

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    @param[in] context The SSL context.
    */
    Secure(IOService & ios, SslContext & context)
        : m_stream(ios, context)
    {}

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Secure> SharedPtr;


    /// @brief The main factory method.
    /**
    @param[in] ios The IO service.
    @param[in] context The SSL context.
    @return The new secure connection.
    */
    static SharedPtr create(IOService & ios, SslContext & context)
    {
        return SharedPtr(new Secure(ios, context));
    }

public:

    /// @brief Get the SSL stream.
    /**
    @return The SSL stream.
    */
    SslStream& getStream()
    {
        return m_stream;
    }

public: // Connection

    /// @copydoc Connection::get_io_service()
    virtual IOService& get_io_service()
    {
        return m_stream.get_io_service();
    }


    /// @copydoc Connection::remote_endpoint()
    virtual Endpoint remote_endpoint() const
    {
        return m_stream.lowest_layer().remote_endpoint();
    }


    /// @copydoc Connection::cancel()
    virtual void cancel()
    {
        ErrorCode terr;

        m_stream.lowest_layer().cancel(terr);
        // ignore error code?
    }


    /// @copydoc Connection::close()
    virtual void close()
    {
        ErrorCode terr;

        m_stream.lowest_layer().shutdown(TcpSocket::shutdown_both, terr);
        // ignore error code?

        m_stream.lowest_layer().close(terr);
        // ignore error code?
    }


    /// @copydoc Connection::async_connect()
    virtual void async_connect(Resolver::iterator epi, ConnectCallback callback)
    {
        // attempt a connection to each endpoint in the list
        boost::asio::async_connect(m_stream.lowest_layer(), epi,
            boost::bind(callback, boost::asio::placeholders::error));
    }


    /// @copydoc Connection::async_handshake()
    virtual void async_handshake(int type, HandshakeCallback callback)
    {
        m_stream.async_handshake(
            boost::asio::ssl::stream_base::handshake_type(type),
            boost::bind(callback, boost::asio::placeholders::error));
    }


    /// @copydoc Connection::async_write_some()
    virtual void async_write_some(ConstBuffers const& bufs, WriteCallback callback)
    {
        m_stream.async_write_some(bufs,
            boost::bind(callback, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @copydoc Connection::async_read_some()
    virtual void async_read_some(MutableBuffers const& bufs, ReadCallback callback)
    {
        m_stream.async_read_some(bufs,
            boost::bind(callback, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:
    SslStream m_stream; ///< @brief The SSL stream.
};

#endif // HIVE_DISABLE_SSL


///////////////////////////////////////////////////////////////////////////////
/// @brief The HTTP client.
/**
It's a wrapper on asynchrounous HTTP request/response.

You can create instance using create() factory method.

This class uses logger called `/hive/http` or `/hive/http/NAME` if client has a non-empty name.

To complete request you have to provide the callback object of Callback signature.

It is possible to specify request's timeout.

All unfinished requests are stored in the internal list and may be cancelled by cancelAll() method.
*/
class Client:
    public boost::enable_shared_from_this<Client>,
    private NonCopyable
{
public:
    typedef boost::system::error_code ErrorCode; ///< @brief The error code.
    typedef boost::asio::io_service IOService; ///< @brief The IO service type.

    typedef boost::asio::ip::tcp::resolver Resolver; ///< @brief The host name resolver.
    typedef boost::asio::ip::tcp::endpoint Endpoint; ///< @brief The endpoint type.
    typedef boost::asio::deadline_timer Timer;       ///< @brief The timer type.

protected:

    /// @brief The main constructor.
    /**
    @param[in] ios The IO service.
    @param[in] name The client name.
    */
    explicit Client(IOService &ios, String const& name)
        : m_ios(ios)
        , m_log("/hive/http/client/" + name)
        , m_nameCache(10*60000) // 10 minutes
#if !defined(HIVE_DISABLE_SSL)
        , m_context(boost::asio::ssl::context::sslv23)
#endif // HIVE_DISABLE_SSL
    {
#if !defined(HIVE_DISABLE_SSL)
        m_context.set_options(boost::asio::ssl::context::default_workarounds);
#endif // HIVE_DISABLE_SSL
        HIVELOG_TRACE_STR(m_log, "created");
    }

public:

    /// @brief The trivial destructor.
    virtual ~Client()
    {
        // TODO: close all keep-alive monitors!
        assert(m_taskList.empty() && "not all tasks are finished");
        HIVELOG_TRACE_STR(m_log, "deleted");
    }

public:

    /// @brief The shared pointer type.
    typedef boost::shared_ptr<Client> SharedPtr;


    /// @brief The main factory method.
    /**
    @param[in] ios The IO service.
    @param[in] name The optional client name.
    @return The new client instance.
    */
    static SharedPtr create(IOService &ios, String const& name = String())
    {
        return SharedPtr(new Client(ios, name));
    }

public:

    /// @brief Get the IO service.
    /**
    @return The IO service.
    */
    IOService& getIoService()
    {
        return m_ios;
    }

public:

    /// @brief The one request/response task.
    /**
    Contains data related to one request/response pair.
    */
    class Task
    {
        friend class Client;
    public:
        typedef boost::shared_ptr<Task> SharedPtr; ///< @brief The shared pointer type.

        RequestPtr    request;      ///< @brief The request object.
        ResponsePtr   response;     ///< @brief The response object.
        ErrorCode     errorCode;    ///< @brief The task error code.

    public:

        /// @brief Get the connection.
        /**
        @return The current connection or `NULL`.
        */
        ConnectionPtr getConnection() const
        {
            return m_connection;
        }


        /// @brief Take the current connection.
        /**
        This method is used to take connection from the HTTP client,
        i.e. prevent the connection caching.

        @return The current connection or `NULL`.
        */
        ConnectionPtr takeConnection()
        {
            ConnectionPtr pconn = m_connection;
            m_connection.reset();
            return pconn;
        }

    public:

        /// @brief The callback method type.
        typedef boost::function0<void> Callback;

        /// @brief Call this method when task is done.
        /**
        This callback will be called when this task is finished (successful or not).

        @param[in] cb The callback to call.
        */
        void callWhenDone(Callback cb)
        {
            if (!m_callback)
                m_callback = cb;
            else
                m_callback = boost::bind(&Task::zcall2, m_callback, cb);
        }

    public:

        /// @brief Cancel all task operations.
        /**
        This method cancels current task.
        */
        void cancel()
        {
            m_cancelled = true;
            m_resolver.cancel();
            if (m_connection)
                m_connection->close();
        }

    private:
        ConnectionPtr m_connection;   ///< @brief The HTTP or HTTPS connection.
        boost::function0<void> m_callback; ///< @brief The callback method.

        bool m_timer_started; ///< @brief The timer "started" flag.
        Timer m_timer;    ///< @brief The deadline timer.

        Resolver m_resolver; ///< @brief The host name resolver.

        bool m_cancelled; ///< @brief The "cancelled" flag.
        size_t m_rx_len; ///< @brief The expected content-length.

    private:

        /// @brief Call two callbacks.
        /**
        @param[in] cb1 The first callback.
        @param[in] cb2 The second callback.
        */
        static void zcall2(Callback cb1, Callback cb2)
        {
            cb1();
            cb2();
        }

    private:

        /// @brief The main constructor.
        /**
        @param[in] ios The IO service.
        @param[in] req The request.
        */
        Task(IOService &ios, RequestPtr req)
            : request(req)
            , m_timer_started(false)
            , m_timer(ios)
            , m_resolver(ios)
            , m_cancelled(false)
            , m_rx_len(0)
        {}
    };

    /// @brief The Task shared pointer type.
    typedef Task::SharedPtr TaskPtr;

public:

    /// @brief Send request asynchronously.
    /**
    @param[in] request The HTTP request to send.
    @param[in] timeout_ms The request timeout, milliseconds.
        If it's zero, no any deadline timers will be started.
    @return The new task instance.
    */
    TaskPtr send(Request::SharedPtr request, size_t timeout_ms)
    {
        HIVELOG_TRACE_BLOCK(m_log, "send()");
        assert(request && "no request");

        // create new task for the request
        TaskPtr task(new Task(m_ios, request));

        if (0 < timeout_ms)
        {
            if (ErrorCode err = asyncStartTimeout(task, timeout_ms))
            {
                HIVELOG_ERROR(m_log, "cannot start deadline timer: ["
                    << err << "] " << err.message());
                return TaskPtr(); // no task started
            }

            HIVELOG_INFO(m_log, "Task{" << task.get() << "} sending "
                << request->getMethod() << " request to <"
                << request->getUrl().toStr() << "> with "
                << timeout_ms << " ms timeout:\n" << *request);
        }
        else
        {
            HIVELOG_INFO(m_log, "Task{" << task.get() << "} sending "
                << request->getMethod() << " request to <"
                << request->getUrl().toStr()
                << "> without timeout:\n" << *request);
        }

        m_taskList.push_back(task);
        asyncResolve(task, true);
        return task;
    }


    /// @brief Cancel all tasks.
    /**
    All active tasks will be finished with `boost::asio::error::operation_aborted` error code.
    */
    void cancelAll()
    {
        HIVELOG_TRACE_BLOCK(m_log, "cancelAll()");
        TaskList::iterator i = m_taskList.begin();
        TaskList::iterator e = m_taskList.end();
        for (; i != e; ++i)
        {
            TaskPtr task = *i;
            task->cancel();
        }
    }


    /// @brief Clear all keep-alive connections.
    /**
    All keep-alive connection will be cancelled and closed.
    */
    void clearKeepAliveConnections()
    {
        HIVELOG_TRACE_BLOCK(m_log, "clearKeepAliveConnections()");
        ConnList::iterator i = m_connCache.begin();
        ConnList::iterator e = m_connCache.end();
        for (; i != e; ++i)
        {
            ConnectionPtr pconn = *i;
            pconn->cancel();
        }

        m_connCache.clear();
    }

private:

    /// @brief Finish the task.
    /**
    Gets the response content from the connection's buffer.

    @param[in] task The task.
    */
    void finish(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "finish(task)");

        Connection::StreamBuf &sbuf = task->m_connection->getBuffer();
        Connection::StreamBuf::const_buffers_type data = sbuf.data();

        if (task->m_rx_len != std::numeric_limits<size_t>::max())
        {
            task->response->setContent(String(
                boost::asio::buffers_begin(data),
                boost::asio::buffers_begin(data) + task->m_rx_len));
            sbuf.consume(task->m_rx_len);
        }
        else // copy whole buffer
        {
            task->response->setContent(String(
                boost::asio::buffers_begin(data),
                boost::asio::buffers_end(data)));
            sbuf.consume(sbuf.size());
        }

        HIVELOG_INFO(m_log, "Task{" << task.get()
            << "} got response:\n"
            << *task->response);
    }


    /// @brief Remove the task.
    /**
    This method stops the task's timer, posts the callback functor
    and removes the task from the active task list.

    @param[in] task The task to remove.
    @param[in] err The error code.
    */
    void done(TaskPtr task, ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "done(task)");

        task->errorCode = err;
        if (task->m_timer_started)
        {
            task->m_timer.cancel();
            task->m_timer_started = false;
        }
        if (Task::Callback cb = task->m_callback)
        {
            task->m_callback = Task::Callback();
            cb(); // call it
        }

        m_taskList.remove(task);

        // cache the connection
        if (ConnectionPtr pconn = task->takeConnection())
        {
            const String hconn = task->response
                               ? task->response->getHeader(http::header::Connection)
                               : String();
            if (boost::iequals(hconn, "keep-alive")) // TODO: or !boost::iequals(hconn, "close")
            {
                m_connCache.push_back(pconn);
                HIVELOG_DEBUG(m_log, "Task{" << task.get()
                    << "} - keep-alive Connection{"
                    << pconn.get() << "} is cached");
                asyncStartKeepAliveMonitor(pconn);
            }
        }
    }

/// @name Task timeout
/// @{
private:

    /// @brief Start asynchronous timeout operation.
    /**
    @param[in] task The task.
    @param[in] timeout_ms The timeout in milliseconds.
    @return The error code.
    */
    ErrorCode asyncStartTimeout(TaskPtr task, size_t timeout_ms)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncStartTimeout(task)");
        ErrorCode err;

        // initialize timer
        task->m_timer.expires_from_now(boost::posix_time::milliseconds(timeout_ms), err);
        if (!err)
        {
            // start it
            task->m_timer.async_wait(
                boost::bind(&Client::onTimedOut, shared_from_this(),
                    task, boost::asio::placeholders::error));
            task->m_timer_started = true;
        }

        return err;
    }


    /// @brief Timeout operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    */
    void onTimedOut(TaskPtr task, ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onTimedOut(task)");

        if (!err) // timeout expired
        {
            HIVELOG_INFO(m_log, "Task{" << task.get() << "} timed out");
            done(task, boost::asio::error::timed_out);
            task->cancel();
        }
        else if (boost::asio::error::operation_aborted == err)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} timeout cancelled");
            // do nothing
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} timeout error: [" << err
                << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Resolve the host name
/// @{
private:

    /// @brief Start asynchronous resolve operation.
    /**
    First attempt uses protocol name as a service,
    the second attempt uses port number.

    @param[in] task The task.
    @param[in] firstAttempt The 'first-attempt' flag.
    */
    void asyncResolve(TaskPtr task, bool firstAttempt)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncResolve(task)");

        Url const& url = task->request->getUrl();
        String service;
        if (firstAttempt) // try the protocol name first
        {
            if (url.getPort().empty())
                service = url.getProtocol();
            else
            {
                // if there is explicit port number provided
                // there is no point to do a second attempt
                service = url.getPort();
                firstAttempt = false;
            }
        }
        else // try the port number otherwise
            service = boost::lexical_cast<String>(url.getPortNumber());

        Endpoint cachedEndpoint;
        const String hostName = url.toStr(Url::PROTOCOL|Url::HOST|Url::PORT);
        if (m_nameCache.enabled() && m_nameCache.find(hostName, cachedEndpoint))
        {
            Resolver::iterator epi = Resolver::iterator::create(cachedEndpoint, url.getHost(), service);
            m_ios.post(boost::bind(&Client::onResolved, shared_from_this(), task, ErrorCode(), epi, firstAttempt));
            HIVELOG_DEBUG(m_log, "Task{" << task.get() << "} resolved from name cache!");
        }
        else
        {
            // start async resolve operation
            HIVELOG_DEBUG(m_log, "Task{" << task.get() << "} start async resolve <"
                << url.getHost() << ">, \"" << service << "\" service");
            task->m_resolver.async_resolve(Resolver::query(url.getHost(), service),
                boost::bind(&Client::onResolved, shared_from_this(),
                    task, boost::asio::placeholders::error,
                    boost::asio::placeholders::iterator,
                        firstAttempt));
        }
    }


    /// @brief Resolve operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    @param[in] epi The endpoint iterator.
    @param[in] firstAttempt The 'first-attempt' flag.
    */
    void onResolved(TaskPtr task, ErrorCode err, Resolver::iterator epi, bool firstAttempt)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onResolved(task)");

        if (!err && !task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get() << "} <"
                << task->request->getUrl().getHost()
                << "> resolved as: " << dump(epi));

            asyncConnect(task, epi);
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async resolve cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else if (firstAttempt)
        {
            HIVELOG_WARN(m_log, "Task{" << task.get() << "} <"
                << task->request->getUrl().getHost()
                << "> async resolve error: ["
                << err << "] " << err.message());
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} resolve with port number "
                   "instead of protocol name");

            // resolve with port number
            // instead of protocol name
            asyncResolve(task, false);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get() << "} <"
                << task->request->getUrl().getHost()
                << "> async resolve error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Connect to the host
/// @{
private:

    /// @brief Find cached connection.
    /**
    @param[in] endpoint The endpoint to find connection for.
    @param[in] secure `true` for secure connections.
    @return A connection or `NULL`.
    */
    ConnectionPtr findCachedConnection(Connection::Endpoint endpoint, bool secure)
    {
        ConnList::iterator i = m_connCache.begin();
        ConnList::iterator e = m_connCache.end();
        for (; i != e; ++i)
        {
            ConnectionPtr pconn = *i;
            if (pconn->remote_endpoint() == endpoint)
            {
#if !defined(HIVE_DISABLE_SSL)
                if (secure)
                {
                    if (boost::dynamic_pointer_cast<Connection::Secure>(pconn))
                    {
                        m_connCache.erase(i);
                        pconn->cancel(); // stop monitor
                        return pconn;
                    }
                }
                else
#endif // HIVE_DISABLE_SSL
                {
                    if (boost::dynamic_pointer_cast<Connection::Simple>(pconn))
                    {
                        m_connCache.erase(i);
                        pconn->cancel(); // stop monitor
                        return pconn;
                    }
                }
            }
        }

        return ConnectionPtr(); // not found
    }


    /// @brief Start asynchronous connect operation.
    /**
    @param[in] task The task.
    @param[in] epi The endpoint iterator.
    */
    void asyncConnect(TaskPtr task, Resolver::iterator epi)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncConnect(task)");
        bool cached = true;

        String const& proto = task->request->getUrl().getProtocol();
        if (boost::iequals(proto, "https") || boost::iequals(proto, "wss")) // secure connection?
        {
#if !defined(HIVE_DISABLE_SSL)
            if (!(task->m_connection = findCachedConnection(epi->endpoint(), true)))
            {
                Connection::Secure::SharedPtr conn = Connection::Secure::create(m_ios, m_context);
                conn->getStream().set_verify_mode(boost::asio::ssl::verify_none); // TODO: boost::asio::ssl::verify_peer
                conn->getStream().set_verify_callback(
                    boost::bind(&Client::onVerify,
                        shared_from_this(), _1, _2));
                task->m_connection = conn;
                cached = false;
            }
#else
            HIVELOG_WARN(m_log, "Task{" << task.get()
                << "} SSL connections not supported");
            done(task, boost::asio::error::operation_not_supported);
            return;
#endif // HIVE_DISABLE_SSL
        }
        else
        {
            if (!(task->m_connection = findCachedConnection(epi->endpoint(), false)))
            {
                task->m_connection = Connection::Simple::create(m_ios);
                cached = false;
            }
        }

        if (cached)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} got Connection{" << task->m_connection.get()
                << "} from cache!");

            m_ios.post(boost::bind(&Client::onHandshaked, shared_from_this(),
                task, boost::system::error_code()));
        }
        else
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} start async Connection{"
                << task->m_connection.get() << "}");

            task->m_connection->async_connect(epi,
                boost::bind(&Client::onConnected, shared_from_this(),
                    task, boost::asio::placeholders::error));
        }
    }


    /// @brief Connect operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    */
    void onConnected(TaskPtr task, ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onConnected(task)");

        if (!err && !task->m_cancelled)
        {
            // TODO: handle Expect 100 header?

            if (m_nameCache.enabled()) // update name cache
            {
                Url const& url = task->request->getUrl();
                const String hostName = url.toStr(Url::PROTOCOL|Url::HOST|Url::PORT);
                m_nameCache.update(hostName, task->m_connection->remote_endpoint());
            }

            asyncHandshake(task);
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async connection cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async connection error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Handshake
/// @{
private:

    /// @brief Start asynchronous handshake operation.
    /**
    @param[in] task The task.
    */
    void asyncHandshake(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncHandshake(task)");

        // send whole request
        HIVELOG_DEBUG(m_log, "Task{" << task.get()
            << "} start async handshake");
        task->m_connection->async_handshake(
#if !defined(HIVE_DISABLE_SSL)
            boost::asio::ssl::stream_base::client,
#else
            0,
#endif // HIVE_DISABLE_SSL
            boost::bind(&Client::onHandshaked, shared_from_this(),
                task, boost::asio::placeholders::error));
    }


    /// @brief Handshake operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    */
    void onHandshaked(TaskPtr task, ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onHandshaked(task)");

        if (!err && !task->m_cancelled)
        {
            asyncWriteRequest(task);
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async handshake cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async handshake error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }

#if !defined(HIVE_DISABLE_SSL)

    /// @brief Verify certificate.
    /**
    @param[in] preverified The preverified flag.
    @param[in] context The verify context.
    @return The "verified" flag.
    */
    bool onVerify(bool preverified, boost::asio::ssl::verify_context& context)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onVerify()");

        HIVELOG_DEBUG(m_log, "verify certificate: " << preverified);

        // TODO: check certificate, use ssl::rfc2818_verification class
        return preverified;
    }

#endif // HIVE_DISABLE_SSL
/// @}

/// @name Send request
/// @{
private:

    /// @brief Start asynchronous request operation.
    /**
    @param[in] task The task.
    */
    void asyncWriteRequest(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncWriteRequest(task)");

        // prepare output buffer
        Connection::StreamBuf &sbuf = task->m_connection->getBuffer();
        OStream os(&sbuf);
        task->request->write(os);

        // send whole request
        HIVELOG_DEBUG(m_log, "Task{" << task.get()
            << "} start async request sending");
        boost::asio::async_write(*task->m_connection, sbuf,
            boost::bind(&Client::onRequestWritten,
                shared_from_this(), task, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief %Request send operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    @param[in] len The number of bytes transferred.
    */
    void onRequestWritten(TaskPtr task, ErrorCode err, size_t len)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onRequestWritten(task)");

        if (!err && !task->m_cancelled)
        {
            asyncReadStatus(task);
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async request sending cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async request sending error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Receive status line
/// @{
private:

    /// @brief Start asynchronous response status operation.
    /**
    @param[in] task The task.
    */
    void asyncReadStatus(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncReadStatus(task)");

        HIVELOG_DEBUG(m_log, "Task{" << task.get()
            << "} start async status line receiving");
        boost::asio::async_read_until(*task->m_connection,
            task->m_connection->getBuffer(), impl::CRLF,
            boost::bind(&Client::onStatusRead, shared_from_this(),
                task, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief %Response status operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    @param[in] len The number of bytes transferred.
    */
    void onStatusRead(TaskPtr task, ErrorCode err, size_t len)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onStatusRead(task)");

        if (!err && !task->m_cancelled)
        {
            std::istreambuf_iterator<char> buf_beg(&task->m_connection->getBuffer());
            const std::istreambuf_iterator<char> buf_end;

            // parse the status line
            int vmajor = 0;
            int vminor = 0;
            int status = 0;
            String reason;
            if (parseStatusLine(buf_beg, buf_end, vmajor, vminor, status, reason))
            {
                task->response = Response::create(status, reason);
                task->response->setVersion(vmajor, vminor);

                HIVELOG_DEBUG(m_log, "Task{" << task.get() << "} got status line: "
                    << dumpStatusLine(task->response));

                // TODO: handle 100-Continue response

                asyncReadHeaders(task);
            }
            else
            {
                HIVELOG_ERROR(m_log, "Task{" << task.get()
                    << "} no data for status line");
                done(task, boost::asio::error::no_data); // boost::asio::error::failure
            }
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async status line receiving cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async status line receiving error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Receive headers
/// @{
private:

    /// @brief Start asynchronous response headers operation.
    /**
    @param[in] task The task.
    */
    void asyncReadHeaders(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncReadHeaders(task)");

        // start "header" reading
        HIVELOG_DEBUG(m_log, "Task{" << task.get()
            << "} start async headers receiving");
        boost::asio::async_read_until(*task->m_connection,
            task->m_connection->getBuffer(), impl::CRLFx2,
            boost::bind(&Client::onHeadersRead, shared_from_this(),
                task, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief %Response headers operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    */
    void onHeadersRead(TaskPtr task, ErrorCode err, size_t)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onHeadersRead(task)");

        if (!err && !task->m_cancelled)
        {
            Connection::StreamBuf &sbuf = task->m_connection->getBuffer();
            std::istreambuf_iterator<char> buf_beg(&sbuf);
            const std::istreambuf_iterator<char> buf_end;

            if (parseHeaders(buf_beg, buf_end, task->response))
            {
                const String len_s = task->response->getHeader(header::Content_Length);
                if (!len_s.empty())
                    task->m_rx_len = boost::lexical_cast<size_t>(len_s);

                // stop if we got all content data
                if (task->m_rx_len <= sbuf.size())
                {
                    finish(task);
                    done(task, err);
                }
                else // continue reading
                    asyncReadContent(task);
            }
            else
            {
                HIVELOG_ERROR(m_log, "Task{" << task.get()
                    << "} no data for headers");
                done(task, boost::asio::error::no_data); // boost::asio::error::failure
            }
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async headers receiving cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async headers receiving error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Receive content
/// @{
private:

    /// @brief Start asynchronous content read operation.
    /**
    @param[in] task The task.
    */
    void asyncReadContent(TaskPtr task)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncReadContent(task)");

        // start "content" reading
        HIVELOG_DEBUG(m_log, "Task{" << task.get()
            << "} start async content receiving");
        boost::asio::async_read(*task->m_connection,
            task->m_connection->getBuffer(),
            boost::asio::transfer_at_least(1),
            boost::bind(&Client::onContentRead,
                shared_from_this(), task, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief %Response content read operation completed.
    /**
    @param[in] task The task.
    @param[in] err The error code.
    */
    void onContentRead(TaskPtr task, ErrorCode err, size_t)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onContentRead(task)");

        Connection::StreamBuf &sbuf = task->m_connection->getBuffer();
        if (!err && !task->m_cancelled)
        {
            // stop if we got all content data
            if (task->m_rx_len <= sbuf.size())
            {
                finish(task);
                done(task, err);
            }
            else // continue reading
                asyncReadContent(task);
        }
        else if (task->m_cancelled)
        {
            HIVELOG_DEBUG(m_log, "Task{" << task.get()
                << "} async content receiving cancelled");
            done(task, boost::asio::error::operation_aborted);
        }
        else if (err == boost::asio::error::eof)
        {
            // clear error if we got the whole content
            if (task->m_rx_len == std::numeric_limits<size_t>::max()
                || task->m_rx_len <= sbuf.size())
                    err = ErrorCode();

            finish(task);
            done(task, err);
        }
        else
        {
            HIVELOG_ERROR(m_log, "Task{" << task.get()
                << "} async content receiving error: ["
                << err << "] " << err.message());
            done(task, err);
        }
    }
/// @}

/// @name Keep-alive connection monitor
/// @{
private:

    /// @brief Start asynchronous keep-alive monitor.
    /**
    @param[in] pconn The keep-alive connection.
    */
    void asyncStartKeepAliveMonitor(ConnectionPtr pconn)
    {
        HIVELOG_TRACE_BLOCK(m_log, "asyncStartKeepAliveMonitor(pconn)");

        HIVELOG_DEBUG(m_log, "Connection{" << pconn.get()
            << "} start async receiving (keep-alive monitor)");
        static char dummy = 0;
        boost::asio::async_read(*pconn, boost::asio::buffer(&dummy, 1),
            boost::bind(&Client::onKeepAliveMonitorRead, shared_from_this(),
                pconn, boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /// @brief Keep-alive monitor operation completed.
    /**
    @param[in] pconn The keep-alive connection.
    @param[in] err The error code.
    @param[in] len The number of bytes transferred.
    */
    void onKeepAliveMonitorRead(ConnectionPtr pconn, ErrorCode err, size_t len)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onKeepAliveMonitorRead(task)");

        if (!err)
        {
            HIVELOG_WARN(m_log, "Connection{" << pconn.get()
                << "} got unexpected data, ignored");
            asyncStartKeepAliveMonitor(pconn);
        }
        else if (err == boost::asio::error::operation_aborted)
        {
            HIVELOG_DEBUG(m_log, "Connection{" << pconn.get()
                << "} async keep-alive monitor cancelled");
        }
        else
        {
            HIVELOG_ERROR(m_log, "Connection{" << pconn.get()
                << "} async keep-alive monitor receiving error: ["
                << err << "] " << err.message());
            HIVELOG_DEBUG(m_log, "Connection{" << pconn.get()
                << "} is dead, will be removed");
            m_connCache.remove(pconn);
        }
    }
/// @}

/// @name Dump tools
/// @{
private:

    /// @brief Dump the endpoints.
    /**
    @param[in] epi The endpoint iterator.
    @return The endpoints dump.
    */
    static String dump(Resolver::iterator epi)
    {
        OStringStream oss;
        oss << "[";

        const Resolver::iterator ie;
        for (Resolver::iterator i = epi; i != ie; ++i)
        {
            const Endpoint endpoint(*i);
            const boost::asio::ip::address addr = endpoint.address();

            if (i != epi)
                oss << ", ";

            oss << addr.to_string() << ":" << endpoint.port()
                << (addr.is_v6() ? "-v6" : (addr.is_v4() ? "-v4" : ""));
        }

        oss << "]";
        return oss.str();
    }


    /// @brief Dump the response status line.
    /**
    @param[in] response The response.
    @return The status line dump.
    */
    static String dumpStatusLine(Response::SharedPtr const& response)
    {
        OStringStream oss;

        if (response)
        {
            oss << "HTTP/"
                << response->getVersionMajor() << "."
                << response->getVersionMinor() << " "
                << response->getStatusCode() << " "
                << response->getStatusPhrase();
        }

        return oss.str();
    }
/// @}

/// @name Parsing tools
/// @{
private:

    /// @brief Parse the status line.
    /**
    @param[in] first The begin of input stream.
    @param[in] last The end of input stream.
    @param[out] vmajor The major version.
    @param[out] vminor The minor version.
    @param[out] status The status code.
    @param[out] reason The reason phrase.
    @return `true` if status line successfully parsed.
    */
    template<typename StreamIterator>
    static bool parseStatusLine(StreamIterator first, StreamIterator last,
        int &vmajor, int &vminor, int &status, String &reason)
    {
        // match for "HTTP/"
        if (first == last || *first++ != 'H')
            return false;
        if (first == last || *first++ != 'T')
            return false;
        if (first == last || *first++ != 'T')
            return false;
        if (first == last || *first++ != 'P')
            return false;
        if (first == last || *first++ != '/')
            return false;

        // parse major version "XXX."
        if (first == last || !misc::is_digit(*first))
            return false;
        vmajor = misc::dec2int(*first++);
        while (first != last && misc::is_digit(*first))
            vmajor = 10*vmajor + misc::dec2int(*first++);
        if (first == last || *first++ != '.')
            return false;

        // parse minor version "XXX "
        if (first == last || !misc::is_digit(*first))
            return false;
        vminor = misc::dec2int(*first++);
        while (first != last && misc::is_digit(*first))
            vminor = 10*vminor + misc::dec2int(*first++);
        if (first == last || *first++ != ' ')
            return false;

        // parse status code "XXX "
        if (first == last || !misc::is_digit(*first))
            return false;
        status = misc::dec2int(*first++);
        while (first != last && misc::is_digit(*first))
            status = 10*status + misc::dec2int(*first++);
        if (first == last || *first++ != ' ')
            return false;

        // parse reason phrase
        while (first != last && *first != '\r')
        {
            if (misc::is_char(*first) && !misc::is_ctl(*first))
                reason.push_back(*first++);
            else
                return false;
        }

        // match for "\r\n"
        if (first == last || *first++ != '\r')
            return false;
        if (first == last || *first++ != '\n')
            return false;

        return true; // OK
    }


    /// @brief Check for the scpecial character.
    /**
    The special characters are: "()<>@,;:\"/[]?={}", space and tab.

    @param[in] ch The input character to test.
    @return `true` if input character is one of the special character.
    */
    static bool is_tspecial(int ch)
    {
        switch (ch)
        {
            case '(': case ')': case '<': case '>': case '@':
            case ',': case ';': case ':': case '\\': case '"':
            case '/': case '[': case ']': case '?': case '=':
            case '{': case '}': case ' ': case '\t':
                return true;
        }

        return false;
    }


    /// @brief Parse the headers.
    /**
    @param[in] first The begin of input stream.
    @param[in] last The end of input stream.
    @param[in,out] response The response with initialized headers.
    @return `false` in case of error.
    */
    template<typename StreamIterator>
    static bool parseHeaders(StreamIterator first, StreamIterator last, Response::SharedPtr & response)
    {
        enum ParserState
        {
            FIRST_HEADER_LINE_START,
            HEADER_LINE_START,
            HEADER_LWS,
            HEADER_NAME,
            SPACE_BEFORE_HEADER_VALUE,
            HEADER_VALUE,
            LINEFEED,
            FINAL_LINEFEED,

            FAIL
        };

        ParserState state = FIRST_HEADER_LINE_START;
        String name, value;

        while (first != last && state != FAIL)
        {
            const char ch = *first++;
            switch (state)
            {
                case FIRST_HEADER_LINE_START:
                    if (ch == '\r')
                        state = FINAL_LINEFEED;
                    else if (!misc::is_char(ch) || misc::is_ctl(ch) || is_tspecial(ch))
                        state = FAIL;
                    else
                    {
                        name.push_back(ch);
                        state = HEADER_NAME;
                    }
                    break;

                case HEADER_LINE_START:
                    if (ch == '\r')
                    {
                        response->addHeader(name, value);
                        name.clear();
                        value.clear();
                        state = FINAL_LINEFEED;
                    }
                    else if (ch == ' ' || ch == '\t')
                        state = HEADER_LWS;
                    else if (!misc::is_char(ch) || misc::is_ctl(ch) || is_tspecial(ch))
                        state = FAIL;
                    else
                    {
                        response->addHeader(name, value);
                        name.clear();
                        value.clear();
                        name.push_back(ch);
                        state = HEADER_NAME;
                    }
                    break;

                case HEADER_LWS:
                    if (ch == '\r')
                        state = LINEFEED;
                    else if (ch == ' ' || ch == '\t')
                        ; // skip it...
                    else if (misc::is_ctl(ch))
                        state = FAIL;
                    else
                    {
                        state = HEADER_VALUE;
                        value.push_back(ch);
                    }
                    break;

                case HEADER_NAME:
                    if (ch == ':')
                        state = SPACE_BEFORE_HEADER_VALUE;
                    else if (!misc::is_char(ch) || misc::is_ctl(ch) || is_tspecial(ch))
                        state = FAIL;
                    else
                        name.push_back(ch);
                    break;

                case SPACE_BEFORE_HEADER_VALUE:
                    state = (ch == ' ') ? HEADER_VALUE : FAIL;
                    break;

                case HEADER_VALUE:
                    if (ch == '\r')
                        state = LINEFEED;
                    else if (misc::is_ctl(ch))
                        state = FAIL;
                    else
                        value.push_back(ch);
                    break;

                case LINEFEED:
                    state = (ch == '\n') ? HEADER_LINE_START : FAIL;
                    break;

                case FINAL_LINEFEED:
                    return (ch == '\n');

                case FAIL: default:
                    return false;
            }
        }

        return false;
    }
/// @}


private:

    /// @name The DNS name cache.
    class NameCache
    {
    public:

        /// @brief Default constructor.
        /**
        @param[in] lifetime_ms The entry lifetime in milliseconds.
        */
        explicit NameCache(size_t lifetime_ms)
            : m_lifetime(lifetime_ms)
        {}


        /// @brief Is enabled?
        /**
        @return `true` if cache is enabled.
        */
        bool enabled() const
        {
            return 0 < m_lifetime;
        }

    public:

        /// @brief Update the entry.
        /**
        @param[in] host The host name.
        @param[in] endpoint The host endpoint.
        */
        void update(String const& host, Endpoint const& endpoint)
        {
            // TODO: do not reset lifetime of existing items?
            m_cache[host] = Entry(endpoint);
        }


        /// @brief Remove the entry.
        /**
        @param[in] host The host name.
        */
        void remove(String const& host)
        {
            m_cache.erase(host);
        }


        /// @brief Get the endpoint from name cache.
        /**
        @param[in] host The host name.
        @param[out] endpoint The host endpoint.
        @return `false` if name entry not found.
        */
        bool find(String const& host, Endpoint &endpoint)
        {
            typedef std::map<String, Entry>::iterator Iterator;
            const Iterator i = m_cache.find(host);
            if (i != m_cache.end())
            {
                const Entry entry = i->second;

                const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
                if ((now - entry.created).total_milliseconds() < m_lifetime)
                {
                    endpoint = i->second.endpoint;
                    return true;
                }
                else
                    m_cache.erase(i); // lifetime expired!
            }

            return false; // not found
        }

    private:

        /// @brief The cache entry.
        struct Entry
        {
            Endpoint endpoint;                  ///< @brief Endpoint address.
            boost::posix_time::ptime created;   ///< @brief Creation time.

            /// @brief The main constructor.
            explicit Entry(Endpoint const& ep)
                : endpoint(ep)
                , created(boost::posix_time::microsec_clock::universal_time())
            {}

            /// @brief The default constructor.
            Entry()
            {}
        };

        std::map<String, Entry> m_cache; ///< @brief The DNS name cache.
        size_t m_lifetime; ///< @brief The DNS name entry lifetime, milliseconds.
    };

private:
    IOService &m_ios; ///< @brief The IO service.
    hive::log::Logger m_log; ///< @brief The HTTP logger.
    NameCache m_nameCache; ///< @brief The local DNS name cache.

#if !defined(HIVE_DISABLE_SSL)
    /// @brief The SSL context.
    Connection::Secure::SslContext m_context;
#endif // HIVE_DISABLE_SSL

    /// @brief The task list type.
    typedef std::list<TaskPtr> TaskList;
    TaskList m_taskList; ///< @brief The task list.

    /// @brief The connection list type.
    typedef std::list<ConnectionPtr> ConnList;
    ConnList m_connCache; ///< @brief The connection cache.
};

/// @brief The HTTP client shared pointer type.
typedef Client::SharedPtr ClientPtr;

    } // http namespace


// HIVE_DISABLE_SSL
#if defined(HIVE_DOXY_MODE)
/// @hideinitializer @brief Disable SSL.
/**
Please define this macro if you don't have OpenSSL.
In that case you will be unable to use https protocol.
*/
#define HIVE_DISABLE_SSL
#endif // defined(HIVE_DOXY_MODE)

} // hive namespace

#endif // __HIVE_HTTP_HPP_


///////////////////////////////////////////////////////////////////////////////
/** @page page_hive_http HTTP module

Using this HTTP module it's possible to send HTTP requests and get
corresponding HTTP responses. All operations are done in asynchronous way.

To use this module you have to form a proper HTTP request (an instance of
hive::http::Request class) and provide the callback function, which will
be called when HTTP request is finished. A HTTP client (an instance of
hive::http::Client class) is used to process all HTTP requests.

The simple example which prints the corresponding request/response
to the standard output:

~~~{.cpp}
using namespace hive;

//// callback: print the request/response to standard output
void on_print(http::Client::TaskPtr task)
{
    if (task->request)
        std::cout << "REQUEST:\n" << *task->request << "\n";
    if (task->response)
        std::cout << "RESPONSE:\n" << *task->response << "\n";
    if (task->errorCode)
        std::cout << "ERROR: " << task->errorCode.message() << "\n";
}

//// application entry point
int main()
{
    boost::asio::io_service ios;

    http::ClientPtr client = http::Client::create(ios); // usually one per application

    http::Url url("http://www.boost.org/LICENSE_1_0.txt");
    http::RequestPtr req = http::Request::GET(url);
    if (http::Client::TaskPtr task = client->send(req, 10000))
        task->callWhenDone(boost::bind(on_print, task));

    ios.run();
    return 0;
}
~~~

You can customize your HTTP request by providing custom HTTP headers
(see hive::http::Message::addHeader() method and hive::http::header namespace)
and message context (see hive::http::Message::setContent() method).

The callback method may be any callable object with the following signature:

~~~{.cpp}
void callback(boost::system::error_code err, hive::http::RequestPtr request, hive::http::ResponsePtr response)
~~~

There are main classes in HTTP module (please see corresponding documentation):
- hive::http::Request
- hive::http::Response
- hive::http::Client
- hive::http::Url

You can use http or https protocols. If you don't have OpenSSL then you can
define #HIVE_DISABLE_SSL macro. In that case you will be unable to use https.

The HTTP module is based on `boost.asio` library. Also the following *boost*
libraries are used: `boost.asio`, `boost.bind`, `boost.system`,
`boost.shared_ptr`, `boost.lexical_cast`, `boost.algorithm`.

Restrictions: no IPv6 yet.
*/
