/** @file
@brief The BTLE gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_gw
*/
#ifndef __EXAMPLES_BLUE_PY_HPP_
#define __EXAMPLES_BLUE_PY_HPP_

#if !defined(HIVE_PCH)
#   include <boost/enable_shared_from_this.hpp>
#   include <boost/algorithm/string.hpp>
#   include <boost/lexical_cast.hpp>
#   include <boost/shared_ptr.hpp>
#   include <boost/asio.hpp>
#   include <boost/bind.hpp>
#endif // HIVE_PCH

#include <hive/defs.hpp>
#include <hive/dump.hpp>
#include <hive/json.hpp>
#include <hive/log.hpp>

#include <sys/types.h>
#include <sys/wait.h>

// bluepy interface
namespace bluepy
{
    using namespace hive;

/**
 * @brief The sub process.
 *
 * Uses pipes for STDERR, STDOUT, STDIN.
 */
class Process:
    public boost::enable_shared_from_this<Process>
{
public:
    typedef boost::function1<void, boost::system::error_code> TerminatedCallback;
    typedef boost::function1<void, String> ReadLineCallback;


    /**
     * @brief Set 'terminated' callback.
     */
    void callWhenTerminated(TerminatedCallback cb)
    {
        m_terminated_cb = cb;
    }


    /**
     * @brief Set 'STDERR read line' callback.
     */
    void callOnNewStderrLine(ReadLineCallback cb)
    {
        m_stderr_line_cb = cb;
    }


    /**
     * @brief Set 'STDERR read line' callback.
     */
    void callOnNewStdoutLine(ReadLineCallback cb)
    {
        m_stdout_line_cb = cb;
    }

protected:

    /**
     * @brief Create sub process.
     * @param ios The IO service.
     * @param argv List of arguments. First item is the executable path.
     */
    Process(boost::asio::io_service &ios, const std::vector<String> &argv, const String &name)
        : m_ios(ios)
        , m_stderr(ios)
        , m_stdout(ios)
        , m_stdin(ios)
        , m_alive(false)
        , m_pid(-1)
        , m_log("bluepy/Process/" + name)
    {
        int pipe_stderr[2];
        int pipe_stdout[2];
        int pipe_stdin[2];

        if (pipe(pipe_stderr) != 0) throw std::runtime_error("cannot create stderr pipe");
        if (pipe(pipe_stdout) != 0) throw std::runtime_error("cannot create stdout pipe");
        if (pipe(pipe_stdin) != 0)  throw std::runtime_error("cannot create stdin pipe");

        m_pid = fork();
        if (m_pid < 0) throw std::runtime_error("cannot create child process");

        if (m_pid) // parent
        {
            // close unused pipe ends
            close(pipe_stderr[1]);
            close(pipe_stdout[1]);
            close(pipe_stdin[0]);

            // assign used pipe ends
            m_stderr.assign(pipe_stderr[0]);
            m_stdout.assign(pipe_stdout[0]);
            m_stdin.assign(pipe_stdin[1]);

            m_alive = true;
        }
        else // child
        {
            // copy used pipe ends
            dup2(pipe_stderr[1], STDERR_FILENO);
            dup2(pipe_stdout[1], STDOUT_FILENO);
            dup2(pipe_stdin[0], STDIN_FILENO);

            // close all pipe ends
            close(pipe_stderr[0]);
            close(pipe_stderr[1]);
            close(pipe_stdout[0]);
            close(pipe_stdout[1]);
            close(pipe_stdin[0]);
            close(pipe_stdin[1]);

            char* c_argv[256];
            size_t c_argc = 0;
            for (; c_argc < argv.size() && c_argc < 255; ++c_argc)
                c_argv[c_argc] = const_cast<char*>(argv[c_argc].c_str());
            c_argv[c_argc++] = 0;

            // execute
            execv(c_argv[0], c_argv);
            exit(-1); // if we are here - something wrong
        }

        HIVELOG_TRACE(m_log, "created");
    }

public:

    /**
     * @brief The factory method.
     */
    static boost::shared_ptr<Process> create(boost::asio::io_service &ios,
                                             const std::vector<String> &argv,
                                             const String &name)
    {
        boost::shared_ptr<Process> pthis(new Process(ios, argv, name));
        pthis->readStderr();
        pthis->readStdout();
        return pthis;
    }


    /**
     * @brief Terminate sub process and get status.
     */
    int terminate()
    {
        HIVELOG_TRACE(m_log, "killing...");
        kill(m_pid, SIGINT);

        int status = 0;
        HIVELOG_TRACE(m_log, "waiting...");
        waitpid(m_pid, &status, 0); // warning: blocking call!

        HIVELOG_INFO(m_log, "status:" << status);
        return status;
    }


    /**
     * @brief Trivial destructor.
     */
    virtual ~Process()
    {
        HIVELOG_TRACE(m_log, "deleted");
    }


    /**
     * @brief Check process is valid/alive.
     */
    bool isAlive() const
    {
        return m_alive;
    }

public:

    /**
     * @brief Write command to the STDIN pipe.
     */
    void writeStdin(const String &cmd)
    {
        bool in_progress = !m_in_queue.empty();
        m_in_queue.push_back(cmd);

        if (!in_progress)
            writeNextStdin();
    }

private:

    /**
     * @brief Write next command from the queue to STDIN pipe.
     */
    void writeNextStdin()
    {
        if (!m_in_queue.empty() && m_alive)
        {
            HIVELOG_INFO(m_log, "writing \"" << escape(m_in_queue.front()) << "\" to STDIN");
            boost::asio::async_write(m_stdin,
                boost::asio::buffer(m_in_queue.front()),
                boost::asio::transfer_all(), boost::bind(&Process::onWriteStdin, shared_from_this(),
                    boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    /**
     * @brief Got some data written into STDIN pipe.
     */
    void onWriteStdin(boost::system::error_code err, size_t len)
    {
        if (!err)
        {
            HIVELOG_TRACE(m_log, "STDIN write " << len << " bytes");

            if (!m_in_queue.empty())
            {
                HIVE_UNUSED(len); // TODO: check (len == m_in_queue.front().size())

                m_in_queue.pop_front();
                if (!m_in_queue.empty())
                {
                    // write next command ASAP
                    m_ios.post(boost::bind(&Process::writeNextStdin,
                                           shared_from_this()));
                }
            }
        }
        else
        {
            HIVELOG_ERROR(m_log, "STDIN write error: ["
                        << err << "] " << err.message());
            handleError(err);
        }
    }

private:

    /**
     * @brief Start asynchronous STDERR reading.
     */
    void readStderr()
    {
        boost::asio::async_read(m_stderr, m_err_buf, boost::asio::transfer_at_least(1),
            boost::bind(&Process::onReadStderr, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }


    /**
     * @brief Start asynchronous STDOUT reading.
     */
    void readStdout()
    {
        boost::asio::async_read(m_stdout, m_out_buf, boost::asio::transfer_at_least(1),
            boost::bind(&Process::onReadStdout, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:

    /**
     * @brief Handle error.
     */
    void handleError(boost::system::error_code err)
    {
        m_alive = false;
        if (m_terminated_cb)
        {
            m_ios.post(boost::bind(m_terminated_cb, err));
            m_terminated_cb = TerminatedCallback(); // send once
        }
    }


    /**
     * @brief Handle STDERR line.
     */
    void handleStderr(const String &line)
    {
        HIVELOG_DEBUG(m_log, "STDERR line: \"" << escape(line) << "\"");
        if (m_stderr_line_cb)
        {
            m_ios.post(boost::bind(m_stderr_line_cb, line));
        }
    }


    /**
     * @brief Handle STDOUT line.
     */
    void handleStdout(const String &line)
    {
        HIVELOG_DEBUG(m_log, "STDOUT line: \"" << escape(line) << "\"");
        if (m_stdout_line_cb)
        {
            m_ios.post(boost::bind(m_stdout_line_cb, line));
        }
    }


    /**
     * @brief Got some data read from STDERR pipe.
     */
    void onReadStderr(boost::system::error_code err, size_t len)
    {
        if (!err)
        {
            HIVELOG_TRACE(m_log, "STDERR read: " << len << " bytes");

            String line;
            m_err_buf.commit(len);
            while (readLine(m_err_buf, line))
                handleStderr(line);

            if (m_alive) // continue
                readStderr();
        }
        else
        {
            HIVELOG_ERROR(m_log, "STDERR read error: ["
                        << err << "] " << err.message());
            handleError(err);
        }
    }


    /**
     * @brief Got some data read from STDOUT pipe.
     */
    void onReadStdout(boost::system::error_code err, size_t len)
    {
        if (!err)
        {
            HIVELOG_TRACE(m_log, "STDOUT read: " << len << " bytes");

            String line;
            m_err_buf.commit(len);
            while (readLine(m_out_buf, line))
                handleStdout(line);

            if (m_alive) // continue
                readStdout();
        }
        else
        {
            HIVELOG_ERROR(m_log, "STDOUT read error: ["
                        << err << "] " << err.message());
            handleError(err);
        }
    }


    /**
     * @brief Try to read line from stream buffer.
     * @return `true` if line has read.
     */
    bool readLine(boost::asio::streambuf &buf, String &line)
    {
        if (size_t pos = readLine_(buf.data(), line))
        {
            buf.consume(pos);
            return true;
        }

        return false;
    }

    template<typename Buffer>
    size_t readLine_(const Buffer &buf, String &line)
    {
        return readLine_(boost::asio::buffers_begin(buf),
                         boost::asio::buffers_end(buf), line);
    }

    template<typename In>
    size_t readLine_(In first, In last, String &line)
    {
        In end = std::find(first, last, '\n');

        if (end != last)
        {
            line.assign(first, end);
            return std::distance(first, end)+1; // including '\n'
        }

        return 0; // not found
    }

public:

    /**
     * @brief Escape non-printable symbols.
     */
    static String escape(const String &str)
    {
        String res;
        res.reserve(str.size());

        for (size_t i = 0; i < str.size(); ++i)
        {
            switch (str[i])
            {
                case '\n': res += "\\n"; break;
                case '\r': res += "\\r"; break;
                default: res += str[i]; break;
            }
        }

        return res;
    }

private:
    boost::asio::io_service &m_ios;

    boost::asio::posix::stream_descriptor m_stderr; // child's STDERR pipe. read end.
    boost::asio::posix::stream_descriptor m_stdout; // child's STDOUT pipe. read end.
    boost::asio::posix::stream_descriptor m_stdin;  // child's STDIN pipe. write end.

    boost::asio::streambuf m_err_buf;
    boost::asio::streambuf m_out_buf;
    std::list<String> m_in_queue;

    bool m_alive;
    pid_t m_pid; // child's PID

    // callbacks
    TerminatedCallback m_terminated_cb;
    ReadLineCallback m_stderr_line_cb;
    ReadLineCallback m_stdout_line_cb;

    log::Logger m_log;
};


/**
 * @brief Process shared pointer type.
 */
typedef boost::shared_ptr<Process> ProcessPtr;


// forward declarations
class Service;
class Characteristic;
class Descriptor;
class Peripheral;

typedef boost::shared_ptr<Service> ServicePtr;
typedef boost::shared_ptr<Characteristic> CharacteristicPtr;
typedef boost::shared_ptr<Descriptor> DescriptorPtr;
typedef boost::shared_ptr<Peripheral> PeripheralPtr;


/**
 * UUID wrapper
 */
class UUID
{
public:
    UUID() // invalid
    {}

    explicit UUID(const String &val)
        : m_val(val)
    {}

    explicit UUID(const UInt32 &val)
        : m_val(dump::hex(val) + "-0000-1000-8000-00805f9b34fb")
    {}

public:

    bool isValid() const
    {
        return !m_val.empty();
    }

    const String& toStr() const
    {
        return m_val;
    }

public:

    bool operator==(const UUID &other) const
    {
        return m_val == other.m_val;
    }

    bool operator<(const UUID &other) const
    {
        return m_val < other.m_val;
    }

public:

    String getCommonName() const
    {
        // TODO: use map here?

        // Service UUIDs
        if (*this == UUID(0x1811)) return "Alert Notification Service";
        if (*this == UUID(0x180F)) return "Battery Service";
        if (*this == UUID(0x1810)) return "Blood Pressure";
        if (*this == UUID(0x1805)) return "Current Time Service";
        if (*this == UUID(0x1818)) return "Cycling Power";
        if (*this == UUID(0x1816)) return "Cycling Speed and Cadence";
        if (*this == UUID(0x180A)) return "Device Information";
        if (*this == UUID(0x1800)) return "Generic Access";
        if (*this == UUID(0x1801)) return "Generic Attribute";
        if (*this == UUID(0x1808)) return "Glucose";
        if (*this == UUID(0x1809)) return "Health Thermometer";
        if (*this == UUID(0x180D)) return "Heart Rate";
        if (*this == UUID(0x1812)) return "Human Interface Device";
        if (*this == UUID(0x1802)) return "Immediate Alert";
        if (*this == UUID(0x1803)) return "Link Loss";
        if (*this == UUID(0x1819)) return "Location and Navigation";
        if (*this == UUID(0x1807)) return "Next DST Change Service";
        if (*this == UUID(0x180E)) return "Phone Alert Status Service";
        if (*this == UUID(0x1806)) return "Reference Time Update Service";
        if (*this == UUID(0x1814)) return "Running Speed and Cadence";
        if (*this == UUID(0x1813)) return "Scan Parameters";
        if (*this == UUID(0x1804)) return "Tx Power";
        if (*this == UUID(0x181C)) return "User Data";

        // Characteristic UUIDs
        if (*this == UUID(0x2A00)) return "Device Name";
        if (*this == UUID(0x2A07)) return "Tx Power Level";
        if (*this == UUID(0x2A19)) return "Battery Level";
        if (*this == UUID(0x2A24)) return "Model Number String";
        if (*this == UUID(0x2A25)) return "Serial Number String";
        if (*this == UUID(0x2A26)) return "Firmware Revision String";
        if (*this == UUID(0x2A27)) return "Hardware Revision String";
        if (*this == UUID(0x2A28)) return "Software Revision String";
        if (*this == UUID(0x2A29)) return "Manufacturer Name String";

        if (boost::iends_with(m_val, "-0000-1000-8000-00805f9b34fb"))
        {
            if (boost::istarts_with(m_val, "0000"))
                return m_val.substr(4, 4);
            else
                return m_val.substr(0, 8);
        }

        return m_val;
    }

private:
    String m_val;
};


/**
 * Service wrapper.
 */
class Service
{
    friend class Peripheral;
public:
    Service(const UUID &uuid, UInt32 start, UInt32 end)
        : m_uuid(uuid)
        , m_start(start)
        , m_end(end)
    {}

public:

    String toStr() const
    {
        OStringStream oss;
        oss << "Service <uuid=" << m_uuid.toStr()
            << " start= " << m_start
            << " end= " << m_end
            << ">";

        return oss.str();
    }

    json::Value toJson() const
    {
        json::Value res;
        res["uuid"] = m_uuid.toStr();
        res["start"] = m_start;
        res["end"] = m_end;

        return res;
    }

private:
    UUID m_uuid;
    UInt32 m_start;
    UInt32 m_end;
};


/**
 * Characteristic wrapper.
 */
class Characteristic
{
    friend class Peripheral;
private:
    Characteristic(const UUID &uuid, UInt32 handle, UInt32 properties, UInt32 valueHandle)
        : m_uuid(uuid)
        , m_handle(handle)
        , m_properties(properties)
        , m_valueHandle(valueHandle)
    {}

public:

    const UUID& getUUID() const
    {
        return m_uuid;
    }

    String toStr() const
    {
        OStringStream oss;
        oss << "Characteristic <uuid=" << m_uuid.toStr()
//            << " handle=" << m_handle
//            << " properties=" << m_properties
//            << " valueHandle=" << m_valueHandle
            << ">";
        return oss.str();
    }

    json::Value toJson() const
    {
        json::Value res;
        res["uuid"] = m_uuid.toStr();
        res["handle"] = m_handle;
        res["properties"] = m_properties;
        res["valueHandle"] = m_valueHandle;

        return res;
    }

private:
    UUID m_uuid;
    UInt32 m_handle;
    UInt32 m_properties;
    UInt32 m_valueHandle;
};


/**
 * Descriptor wrapper.
 */
class Descriptor
{
    friend class Peripheral;
private:
    Descriptor(const UUID &uuid, UInt32 handle)
        : m_uuid(uuid)
        , m_handle(handle)
    {}

public:
    String toStr() const
    {
        OStringStream oss;
        oss << "Descriptor <uuid=" << m_uuid.toStr()
            //<< " handle=" << m_handle
            << ">";
        return oss.str();
    }

    json::Value toJson() const
    {
        json::Value res;
        res["uuid"] = m_uuid.toStr();
        res["handle"] = m_handle;

        return res;
    }

private:
    UUID m_uuid;
    UInt32 m_handle;
};


/**
 * Peripheral wrapper.
 */
class Peripheral
        : public boost::enable_shared_from_this<Peripheral>
{
private:
    Peripheral(boost::asio::io_service &ios, const String &helperPath, const String &deviceAddr)
        : m_ios(ios)
        , m_helperPath(helperPath)
        , m_deviceAddr(deviceAddr)
        , m_discoveredAllServices(false)
        , m_log("/bluepy/Peripheral/" + deviceAddr)
    {
        HIVELOG_TRACE(m_log, "created");
    }

public:

    /**
     * @brief Factory method.
     */
    static PeripheralPtr create(boost::asio::io_service &ios, const String &helperPath, const String &deviceAddr)
    {
        return PeripheralPtr(new Peripheral(ios, helperPath, deviceAddr));
    }


    /**
     * @brief Destructor.
     */
    ~Peripheral()
    {
        stop();

        HIVELOG_TRACE(m_log, "deteted");
    }


    /**
     * @brief Stop helper.
     */
    void stop()
    {
        stopHelper();
        m_terminated_cb = TerminatedCallback();
    }


    /**
     * @brief Get MAC address.
     */
    const String& getAddress() const
    {
        return m_deviceAddr;
    }

public:

    typedef boost::function1<void, boost::system::error_code> TerminatedCallback;

    /**
     * @brief Set 'terminated' callback.
     */
    void callWhenTerminated(TerminatedCallback cb)
    {
        m_terminated_cb = cb;
    }

private:
    TerminatedCallback m_terminated_cb;


    /**
     * @brief Start helper if not started yet.
     */
    void startHelper()
    {
        if (!m_helper)
        {
            std::vector<String> argv;
            argv.push_back(m_helperPath);

            HIVELOG_INFO(m_log, "executing helper: " << m_helperPath);
            m_helper = Process::create(m_ios, argv, m_deviceAddr);

            // attach
            m_helper->callWhenTerminated(boost::bind(&Peripheral::onHelperTerminated,
                shared_from_this(), _1));
            m_helper->callOnNewStderrLine(boost::bind(&Peripheral::onParseStdout,
                shared_from_this(), _1, true));
            m_helper->callOnNewStdoutLine(boost::bind(&Peripheral::onParseStdout,
                shared_from_this(), _1, false));
        }
    }


    /**
     * @brief Stop helper if started.
     */
    void stopHelper()
    {
        if (m_helper)
        {
            HIVELOG_INFO(m_log, "terminating helper...");

            if (m_helper->isAlive())
                m_helper->writeStdin("quit\n");
            m_helper->terminate();
            m_helper.reset();
        }
    }


    /**
     * @brief Helper is terminated.
     */
    void onHelperTerminated(boost::system::error_code err)
    {
        HIVELOG_DEBUG(m_log, "helper terminated: [" << err << "]: " << err.message());
        if (m_terminated_cb)
        {
            m_ios.post(boost::bind(m_terminated_cb, err));
            m_terminated_cb = TerminatedCallback(); // once
        }

        m_helper.reset();
    }

public:

    /**
     * @brief Parse helper's response line.
     * @param line The response line.
     * @return The response as JSON.
     */
    static json::Value parseResp(const String &line)
    {
        json::Value resp;

        IStringStream iss(line);
        while (!iss.eof())
        {
            String item;
            if (!(iss >> item))
                break;

            if (item.empty())
                continue;

            String tag, tval;
            size_t n = item.find('=');
            if (n != String::npos)
            {
                tag = item.substr(0, n);
                tval = item.substr(n+1);
            }
            else
                tag = item;

            json::Value val;
            if (tval.empty())
                val = json::Value::null();
            else if (tval[0]=='$' || tval[0]=='\'')
                val = tval.substr(1); // ignore string mark
            else if (tval[0]=='h')
                val = UInt32(strtoul(tval.substr(1).c_str(), 0, 16));
            else if (tval[0]=='b')
                val = tval.substr(1); // hex string
            else
                throw std::runtime_error("Cannot understand response value " + tval);

            if (!resp.hasMember(tag))       // create new
                resp[tag] = val;
            else                            // already exists
            {
                json::Value &oldval = resp[tag];

                if (oldval.isArray())       // append to array
                    oldval.append(val);
                else                        // create array
                {
                    json::Value newval(json::Value::TYPE_ARRAY);
                    newval.append(oldval);
                    newval.append(val);
                    oldval = newval;
                }
            }
        }

        return resp;
    }

private:

    /**
     * @brief The abstract Command class.
     */
    class Command
    {
    protected:
        Command() {}

    public:
        virtual ~Command() {}

        virtual String getCommand() const { return String(); }
        virtual bool wantProcess(const String&) const { return false; }
        virtual bool process(const String &resp, const json::Value &data) = 0;

    public:
        class Status;
        class Connect;
        class Disconnect;
        class Services;
        class Characteristics;
        class CharRead;
        class CharWrite;
    };

    typedef boost::shared_ptr<Command> CommandPtr;
    std::list<CommandPtr> m_commands;


    /**
     * @brief Write command to the command queue.
     */
    void writeCmd(const CommandPtr &cmd)
    {
        bool is_progress = !m_commands.empty();
        m_commands.push_back(cmd);
        HIVELOG_TRACE(m_log, "putting \"" << Process::escape(cmd->getCommand()) << "\" to the BACK of command queue");

        if (!is_progress)
            writeNextCommand();
    }

    /**
     * @brief Write command to the front of command queue!
     */
    void writeCmdFront(const CommandPtr &cmd)
    {
        bool is_progress = !m_commands.empty();
        m_commands.push_front(cmd);
        HIVELOG_TRACE(m_log, "putting \"" << Process::escape(cmd->getCommand()) << "\" to the FRONT of command queue");

        if (!is_progress)
            writeNextCommand();
    }


    /**
     * @brief Write next command from the command queue to the helper.
     */
    void writeNextCommand()
    {
        if (m_helper && !m_commands.empty())
        {
            String cmd = m_commands.front()->getCommand();

            HIVELOG_DEBUG(m_log, "sending \"" << Process::escape(cmd) << "\" to STDIN");
            m_helper->writeStdin(cmd);
        }
    }

    /**
     * @brief Parse helper's STDOUT or STDERR line.
     */
    void onParseStdout(const String &line, bool isStdErr)
    {
        if (boost::starts_with(line, "#"))
        {
            HIVELOG_DEBUG(m_log, "ignore comment: " << line);
            return;
        }

        if (isStdErr)
        {
            HIVELOG_WARN(m_log, "ignore STDERR: " << line);
            return;
        }

        HIVELOG_DEBUG(m_log, "got \"" << Process::escape(line) << "\" on "
                      << (isStdErr ? "STDERR":"STDOUT") << " to parse");

        try
        {
            json::Value data = Peripheral::parseResp(line);
            HIVELOG_DEBUG(m_log, "response parsed as " << json::toStr(data));

            String rsp = data["rsp"].asString();
            if (rsp.empty()) throw std::runtime_error("no response type indicator");

            if (!m_commands.empty() && m_commands.front()->wantProcess(rsp))
            {
                CommandPtr cmd = m_commands.front();
                if (cmd->process(rsp, data))
                {
                    m_commands.remove(cmd); // cmd might not be the first
                    if (!m_commands.empty())
                        writeNextCommand();
                }
            }
//            else if (rsp == "stat" && data["state"].asString() == "disc")
//            {
//                throw std::runtime_error("device disconnected");
//            }
            else if (rsp == "err")
            {
                throw std::runtime_error("Bluetooth error: " + data["code"].asString());
                // TODO: process error
            }
            else if (rsp == "ntfy")
            {
                processNotification(data);
            }
            else
            {
                HIVELOG_WARN(m_log, "unexpected response, ignored");
            }
        }
        catch (const std::exception &ex)
        {
            HIVELOG_ERROR(m_log, "failed to parse line \"" << Process::escape(line)
                          << "\": " << ex.what());
            // TODO: reset?
        }
    }

public:
    typedef boost::function2<void, UInt32, String> NotificationCallback;

    void callOnNewNotification(NotificationCallback cb)
    {
        m_notification_cb = cb;
    }

private:
    NotificationCallback m_notification_cb;

    /**
     * @brief Process notifications.
     */
    void processNotification(const json::Value &data)
    {
        UInt32 handle = data["hnd"].asUInt32();
        String value = data["d"].asString(); // hex

        if (m_notification_cb)
        {
            m_ios.post(boost::bind(m_notification_cb, handle, value));
        }
    }


public: // status

    // [status]: 'conn' or 'disc'
    typedef boost::function1<void, String> StatusCallback;

    /**
     * @brief The Status command.
     */
    class Command::Status: public Command
    {
    protected:
        Status() {}

    public:
        static boost::shared_ptr<Status> create(StatusCallback cb)
        {
            boost::shared_ptr<Status> pthis(new Status());
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return "stat\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "stat");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            String state = data["state"].asString();
            if (m_cb) m_cb(state);
            return true;
        }

    private:
        StatusCallback m_cb;
    };


    /**
     * @brief Check status asynchronously.
     * @param cb Optional callback.
     */
    void status(StatusCallback cb = StatusCallback())
    {
        if (!m_helper)
        {
            if (cb) // report disconnected state
                m_ios.post(boost::bind(cb, String("disc")));
            return;
        }

        writeCmd(Command::Status::create(cb));
    }

private:

    typedef boost::function0<void> Callback;

    /**
     * @brief Call 'ok' if status is 'conn' or 'fail' otherwise.
     */
    void onStatusConn(const String &state, Callback ok, Callback fail)
    {
        if (boost::iequals(state, "conn"))
        {
            if (ok)
                ok();
        }
        else
        {
            if (fail)
                fail();
        }
    }

    /**
     * @brief Call 'ok' if status is 'disc' or 'fail' otherwise.
     */
    void onStatusDisc(const String &state, Callback ok, Callback fail)
    {
        if (boost::iequals(state, "disc"))
        {
            if (ok)
                ok();
        }
        else
        {
            if (fail)
                fail();
        }
    }

public: // connect

    typedef boost::function1<void, bool> ConnectCallback;

    /**
     * @brief The Connect command.
     */
    class Command::Connect: public Command
    {
    protected:
        Connect() {}

    public:
        static boost::shared_ptr<Connect> create(const String &addr, ConnectCallback cb)
        {
            boost::shared_ptr<Connect> pthis(new Connect());
            pthis->m_addr = addr;
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return  "conn " + m_addr + "\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "stat");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            String state = data["state"].asString();

            if (boost::iequals(state, "tryconn"))
                return false; // wait...

            if (boost::iequals(state, "conn"))
            {
                if (m_cb) m_cb(true); // connected
                return true; // OK
            }

            if (m_cb) m_cb(false); // disconnected
            return true;
        }

    private:
        ConnectCallback m_cb;
        String m_addr;
    };


    /**
     * @brief Connect asynchronously.
     * @param cb Optional callback.
     */
    void connect(ConnectCallback cb = ConnectCallback())
    {
        startHelper();

        // do nothing if status is 'conn' (report connected)
        Callback ok = cb ? boost::bind(cb, true) : Callback();

        // otherwise send Connect command
        CommandPtr cmd = Command::Connect::create(m_deviceAddr, cb);
        Callback fail = boost::bind(&Peripheral::writeCmdFront,
                                shared_from_this(), cmd);

        // check status first, then connect
        status(boost::bind(&Peripheral::onStatusConn,
            shared_from_this(), _1, ok, fail));
    }

private:

    /**
     * @brief Call 'ok' if connected or 'fail' otherwise.
     */
    void onConnected(bool connected, Callback ok, Callback fail)
    {
        if (connected)
        {
            if (ok)
                ok();
        }
        else
        {
            if (fail)
                fail();
        }
    }

public: // disconnect

    /**
     * @brief The Disconnect command.
     */
    class Command::Disconnect: public Command
    {
    protected:
        Disconnect() {}

    public:
        static boost::shared_ptr<Disconnect> create(ConnectCallback cb)
        {
            boost::shared_ptr<Disconnect> pthis(new Disconnect());
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return  "disc\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "stat");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            String state = data["state"].asString();

            if (boost::iequals(state, "disc"))
            {
                if (m_cb) m_cb(false); // disconnected
                return true; // OK
            }

            if (m_cb) m_cb(true); // still connected
            return true;
        }

    private:
        ConnectCallback m_cb;
    };


    /**
     * @brief Disconnect asynchronously.
     */
    void disconnect(ConnectCallback cb = ConnectCallback())
    {
        if (!m_helper)
        {
            if (cb) // report disconnected
                m_ios.post(boost::bind(cb, false));
            return;
        }

        // do nothing if status is 'disc' (report disconnected)
        Callback ok = cb ? boost::bind(cb, false) : Callback();

        // otherwise send Disconnect command
        CommandPtr cmd = Command::Disconnect::create(cb);
        Callback fail = boost::bind(&Peripheral::writeCmdFront,
                            shared_from_this(), cmd);

        // check status first, then disconnect
        status(boost::bind(&Peripheral::onStatusDisc,
            shared_from_this(), _1, ok, fail));
    }

public:

    // [status] [list of services]
    typedef boost::function2<void, String, std::vector<ServicePtr> > ServicesCallback;


    /**
     * @brief The Services command.
     */
    class Command::Services: public Command
    {
    protected:
        Services() {}

    public:
        static boost::shared_ptr<Services> create(ServicesCallback cb)
        {
            boost::shared_ptr<Services> pthis(new Services());
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return "svcs\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "find");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            json::Value starts = data["hstart"];
            json::Value ends   = data["hend"];
            json::Value uuids  = data["uuid"];

            std::vector<ServicePtr> services;
            if (uuids.isArray())
            {
                for (size_t i = 0; i < uuids.size(); ++i)
                {
                    services.push_back(ServicePtr(new Service(UUID(uuids[i].asString()),
                                                              starts[i].asUInt32(),
                                                              ends[i].asUInt32())));
                }
            }
            else
            {
                services.push_back(ServicePtr(new Service(UUID(uuids.asString()),
                                                          starts.asUInt32(),
                                                          ends.asUInt32())));
            }

            if (m_cb) m_cb(String(), services);
            return true;
        }

    private:
        ServicesCallback m_cb;
    };


    /**
     * @brief Get services asynchronously.
     */
    void services(ServicesCallback cb = ServicesCallback())
    {
        startHelper();

        // send Services command if connected
        CommandPtr cmd = Command::Services::create(cb);
        Callback ok = boost::bind(&Peripheral::writeCmdFront,
                          shared_from_this(), cmd);

        // otherwise report error
        Callback fail = cb ? boost::bind(cb, String("Disconnected"),
                                         std::vector<ServicePtr>())
                           : Callback();

        // connect first, then send Services
        connect(boost::bind(&Peripheral::onConnected,
            shared_from_this(), _1, ok, fail));
    }

public:

    // [status] [list of characteristics]
    typedef boost::function2<void, String, std::vector<CharacteristicPtr> > CharacteristicsCallback;


    /**
     * @brief The Characteristics command.
     */
    class Command::Characteristics: public Command
    {
    protected:
        Characteristics() {}

    public:
        static boost::shared_ptr<Characteristics> create(UInt32 start, UInt32 end, const UUID &uuid,
                                                         CharacteristicsCallback cb)
        {
            boost::shared_ptr<Characteristics> pthis(new Characteristics());

            pthis->m_cmd = "char ";
            pthis->m_cmd += dump::hex(start);
            pthis->m_cmd += " ";
            pthis->m_cmd += dump::hex(end);
            if (uuid.isValid())
            {
                pthis->m_cmd += " ";
                pthis->m_cmd += uuid.toStr();
            }
            pthis->m_cmd += "\n";

            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return m_cmd;
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "find");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            json::Value hnd = data["hnd"];
            json::Value uuid = data["uuid"];
            json::Value prop = data["props"];
            json::Value vhnd = data["vhnd"];

            std::vector<CharacteristicPtr> chars;
            if (uuid.isArray())
            {
                for (size_t i = 0; i < uuid.size(); ++i)
                {
                    chars.push_back(CharacteristicPtr(new Characteristic(UUID(uuid[i].asString()),
                                                                         hnd[i].asUInt32(),
                                                                         prop[i].asUInt32(),
                                                                         vhnd[i].asUInt32())));
                }
            }
            else
            {
                chars.push_back(CharacteristicPtr(new Characteristic(UUID(uuid.asString()),
                                                                     hnd.asUInt32(),
                                                                     prop.asUInt32(),
                                                                     vhnd.asUInt32())));
            }

            if (m_cb) m_cb(String(), chars);
            return true;
        }

    private:
        CharacteristicsCallback m_cb;
        String m_cmd;
    };

    void characteristics(CharacteristicsCallback cb = CharacteristicsCallback(),
                         UInt32 start=0x0001, UInt32 end=0xFFFF, const UUID &uuid = UUID())
    {
        startHelper();

        // send Characteristics command if connected
        CommandPtr cmd = Command::Characteristics::create(start, end, uuid, cb);
        Callback ok = boost::bind(&Peripheral::writeCmdFront,
                          shared_from_this(), cmd);

        // otherwise report error
        Callback fail = cb ? boost::bind(cb, String("Disconnected"),
                                         std::vector<CharacteristicPtr>())
                           : Callback();

        // connect first, then send Characteristics
        connect(boost::bind(&Peripheral::onConnected,
            shared_from_this(), _1, ok, fail));
    }

public:

    // [status] [hex value]
    typedef boost::function2<void, String, String> CharReadCallback;


    /**
     * @brief The CharRead command.
     */
    class Command::CharRead: public Command
    {
    protected:
        CharRead(): m_handle(0) {}

    public:
        static boost::shared_ptr<CharRead> create(UInt32 handle, CharReadCallback cb)
        {
            boost::shared_ptr<CharRead> pthis(new CharRead());
            pthis->m_handle = handle;
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return "rd " + dump::hex(m_handle) + "\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "rd");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            json::Value d = data["d"];
            if (m_cb) m_cb(String(), d.asString());
            return true;
        }

    private:
        CharReadCallback m_cb;
        UInt32 m_handle;
    };


    /**
     * @brief Read characteristic asynchronously.
     */
    void readChar(UInt32 handle, CharReadCallback cb = CharReadCallback())
    {
        startHelper();

        // send CharRead command if connected
        CommandPtr cmd = Command::CharRead::create(handle, cb);
        Callback ok = boost::bind(&Peripheral::writeCmdFront,
                          shared_from_this(), cmd);

        // otherwise report error
        Callback fail = cb ? boost::bind(cb, String("Disconnected"), String())
                           : Callback();

        // connect first, then send CharRead
        connect(boost::bind(&Peripheral::onConnected,
            shared_from_this(), _1, ok, fail));
    }

//    def _readCharacteristicByUUID(self, uuid, startHnd, endHnd):
//        # Not used at present
//        self._writeCmd("rdu %s %X %X\n" % (UUID(uuid), startHnd, endHnd))
//        return self._getResp('rd')

public:

    // [status]
    typedef boost::function1<void, String> CharWriteCallback;


    /**
     * @brief The CharWrite command.
     */
    class Command::CharWrite: public Command
    {
    protected:
        CharWrite(): m_handle(0), m_withResp(false) {}

    public:
        static boost::shared_ptr<CharWrite> create(UInt32 handle, const String &val, bool withResp, CharWriteCallback cb)
        {
            boost::shared_ptr<CharWrite> pthis(new CharWrite());
            pthis->m_withResp = withResp;
            pthis->m_handle = handle;
            pthis->m_val = val;
            pthis->m_cb = cb;
            return pthis;
        }

        virtual String getCommand() const
        {
            return (m_withResp ? "wrr ":"wr ")
                 + dump::hex(m_handle)
                 + " " + m_val + "\n";
        }

        virtual bool wantProcess(const String &resp) const
        {
            return boost::iequals(resp, "wr");
        }

        virtual bool process(const String &resp, const json::Value &data)
        {
            if (!wantProcess(resp))
                return false;

            HIVE_UNUSED(data);
            if (m_cb) m_cb(String()); // ok
            return true;
        }

    private:
        CharWriteCallback m_cb;
        UInt32 m_handle;
        bool m_withResp;
        String m_val;
    };


    /**
     * @brief Write characteristic asynchronously.
     */
    void writeChar(UInt32 handle, const String &val, bool withResp = false, CharWriteCallback cb = CharWriteCallback())
    {
        startHelper();

        // send CharWrite command if connected
        CommandPtr cmd = Command::CharWrite::create(handle, val, withResp, cb);
        Callback ok = boost::bind(&Peripheral::writeCmdFront,
                          shared_from_this(), cmd);

        // otherwise report error
        Callback fail = cb ? boost::bind(cb, String("Disconnected"))
                           : Callback();

        // connect first, then send CharWrite
        connect(boost::bind(&Peripheral::onConnected,
            shared_from_this(), _1, ok, fail));
    }

public:

//    def setSecurityLevel(self, level):
//        self._writeCmd("secu %s\n" % level)
//        return self._getResp('stat')

//    def setMTU(self, mtu):
//        self._writeCmd("mtu %x\n" % mtu)
//        return self._getResp('stat')


private:
    boost::asio::io_service &m_ios;
    String m_helperPath;

    ProcessPtr m_helper;
    String m_deviceAddr;
    bool m_connected;

    bool m_discoveredAllServices;
    log::Logger m_log;
};

} // bluepy namespace

#endif // __EXAMPLES_BLUE_PY_HPP_
