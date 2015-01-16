/** @file
@brief The BTLE gateway example.
@author Sergey Polichnoy <sergey.polichnoy@dataart.com>
@see @ref page_simple_gw
*/
#ifndef __EXAMPLES_BTLE_GW_HPP_
#define __EXAMPLES_BTLE_GW_HPP_

#include <DeviceHive/restful.hpp>
#include <DeviceHive/websocket.hpp>
#include "basic_app.hpp"

// 'libbluetooth-dev' should be installed
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/// @brief The BTLE gateway example.
namespace btle_gw
{
    using namespace hive;

/// @brief Various contants and timeouts.
enum Timeouts
{
    STREAM_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open stream device each X milliseconds.
    SERVER_RECONNECT_TIMEOUT    = 10000, ///< @brief Try to open server connection each X milliseconds.
    RETRY_TIMEOUT               = 5000,  ///< @brief Common retry timeout, milliseconds.
    DEVICE_OFFLINE_TIMEOUT      = 5
};


/// @brief Execute OS command.
static String shellExec(const String &cmd)
{
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("unable to execute command");

    String result;
    while (!feof(pipe))
    {
        char buf[256];
        if (fgets(buf, sizeof(buf), pipe) != NULL)
            result += buf;
    }
    /*int ret = */pclose(pipe);

    boost::trim(result);
    return result;
}


/// @brief The simple gateway application.
/**
This application controls only one device connected via serial port or socket or pipe!

@see @ref page_simple_gw
*/
class Application:
    public basic_app::Application,
    public devicehive::IDeviceServiceEvents
{
    typedef basic_app::Application Base; ///< @brief The base type.
    typedef Application This; ///< @brief The type alias.

public:

    /// @brief Bluetooth device.
    class BluetoothDevice:
        public boost::enable_shared_from_this<BluetoothDevice>,
        private hive::NonCopyable
    {
    protected:

        /// @brief Main constructor.
        BluetoothDevice(boost::asio::io_service &ios, const String &name)
            : scan_filter_dup(0x01)
            , scan_filter_type(0)
            , scan_filter_old_valid(false)
            , m_scan_active(false)
            , m_read_active(false)
            , m_ios(ios)
            , m_name(name)
            , m_dev_id(-1)
            , m_dd(-1)
            , m_stream(ios)
        {
            memset(&m_dev_addr, 0, sizeof(m_dev_addr));
        }

    public:
        typedef boost::shared_ptr<BluetoothDevice> SharedPtr;

        /// @brief The factory method.
        static SharedPtr create(boost::asio::io_service &ios, const String &name)
        {
            return SharedPtr(new BluetoothDevice(ios, name));
        }

    public:

        /// @brief Get IO service.
        boost::asio::io_service& get_io_service()
        {
            return m_ios;
        }


        /// @brief Get bluetooth device name.
        const String& getDeviceName() const
        {
            return m_name;
        }


        /// @brief Get device identifier.
        int getDeviceId() const
        {
            return m_dev_id;
        }


        /// @brief Get bluetooth address as string.
        String getDeviceAddressStr() const
        {
            char str[64];
            ba2str(&m_dev_addr, str);
            return String(str);
        }


        /// @brief Get device info.
        json::Value getDeviceInfo() const
        {
            hci_dev_info info;
            memset(&info, 0, sizeof(info));

            if (hci_devinfo(m_dev_id, &info) < 0)
                throw std::runtime_error("cannot get device info");

            return info2json(info);
        }


        /// @brief Convert device info to JSON value.
        static json::Value info2json(const hci_dev_info &info)
        {
            json::Value res;
            res["id"] = (int)info.dev_id;
            res["name"] = String(info.name);

            char *flags = hci_dflagstostr(info.flags);
            res["flags"] = boost::trim_copy(String(flags));
            bt_free(flags);

            char addr[64];
            ba2str(&info.bdaddr, addr);
            res["addr"] = String(addr);

            return res;
        }

        /// @brief Get info for all devices.
        static json::Value getDevicesInfo()
        {
            struct Aux
            {
                static int collect(int, int dev_id, long arg)
                {
                    hci_dev_info info;
                    memset(&info, 0, sizeof(info));

                    if (hci_devinfo(dev_id, &info) < 0)
                        throw std::runtime_error("cannot get device info");

                    json::Value *res = reinterpret_cast<json::Value*>(arg);
                    res->append(info2json(info));

                    return 0;
                }
            };

            json::Value res(json::Value::TYPE_ARRAY);
            hci_for_each_dev(0, Aux::collect,
                             reinterpret_cast<long>(&res));
            return res;
        }

    public:
        uint8_t scan_filter_dup;
        uint8_t scan_filter_type;
        hci_filter scan_filter_old;
        bool scan_filter_old_valid;

        /**
         * @brief Start scan operation.
         */
        void scanStart(const json::Value &opts)
        {
            uint8_t own_type = LE_PUBLIC_ADDRESS;
            uint8_t scan_type = 0x01;
            uint8_t filter_policy = 0x00;
            uint16_t interval = htobs(0x0010);
            uint16_t window = htobs(0x0010);

            const json::Value &j_dup = opts["duplicates"];
            if (!j_dup.isNull())
            {
                if (j_dup.isConvertibleToInteger())
                    scan_filter_dup = (j_dup.asInt() != 0);
                else
                {
                    String s = j_dup.asString();
                    if (boost::iequals(s, "yes"))
                        scan_filter_dup = 0x00;     // don't filter - has duplicates
                    else if (boost::iequals(s, "no"))
                        scan_filter_dup = 0x01;     // filter - no duplicates
                    else
                        throw std::runtime_error("unknown duplicates value");
                }
            }
            else
                scan_filter_dup = 0x01; // filter by default

            const json::Value &j_priv = opts["privacy"];
            if (!j_priv.isNull())
            {
                if (j_priv.isConvertibleToInteger())
                {
                    if (j_priv.asInt())
                        own_type = LE_RANDOM_ADDRESS;
                }
                else
                {
                    String s = j_priv.asString();

                    if (boost::iequals(s, "enable") || boost::iequals(s, "enabled"))
                        own_type = LE_RANDOM_ADDRESS;
                    else if (boost::iequals(s, "disable") || boost::iequals(s, "disabled"))
                        ;
                    else
                        throw std::runtime_error("unknown privacy value");
                }
            }

            const json::Value &j_type = opts["type"];
            if (!j_type.isNull())
            {
                if (j_type.isConvertibleToInteger())
                    scan_type = j_type.asUInt8();
                else
                {
                    String s = j_type.asString();

                    if (boost::iequals(s, "active"))
                        own_type = 0x01;
                    else if (boost::iequals(s, "passive"))
                        own_type = 0x00;
                    else
                        throw std::runtime_error("unknown scan type value");
                }
            }

            const json::Value &j_pol = opts["policy"];
            if (!j_pol.isNull())
            {
                if (j_pol.isConvertibleToInteger())
                    filter_policy = j_pol.asUInt8();
                else
                {
                    String s = j_pol.asString();

                    if (boost::iequals(s, "whitelist"))
                        filter_policy = 0x01;
                    else if (boost::iequals(s, "none"))
                        filter_policy = 0x00;
                    else
                        throw std::runtime_error("unknown filter policy value");
                }
            }

//switch (opt) {
//case 'd':
//    filter_type = optarg[0];
//    if (filter_type != 'g' && filter_type != 'l') {
//        fprintf(stderr, "Unknown discovery procedure\n");
//        exit(1);
//    }

//    interval = htobs(0x0012);
//    window = htobs(0x0012);
//    break;
//}

            int err = hci_le_set_scan_parameters(m_dd, scan_type, interval, window,
                                                 own_type, filter_policy, 10000);
            if (err < 0) throw std::runtime_error("failed to set scan parameters");

            err = hci_le_set_scan_enable(m_dd, 0x01, scan_filter_dup, 10000);
            if (err < 0) throw std::runtime_error("failed to enable scan");
            m_scan_active = true;

            socklen_t len = sizeof(scan_filter_old);
            err = getsockopt(m_dd, SOL_HCI, HCI_FILTER, &scan_filter_old, &len);
            if (err < 0) throw std::runtime_error("failed to get filter option");
            scan_filter_old_valid = true;

            hci_filter nf;
            hci_filter_clear(&nf);
            hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
            hci_filter_set_event(EVT_LE_META_EVENT, &nf);

            err = setsockopt(m_dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf));
            if (err < 0) throw std::runtime_error("failed to set filter option");

            m_scan_devices.clear(); // new search...
        }


        /**
         * @brief Stop scan operation.
         */
        void scanStop()
        {
            if (scan_filter_old_valid) // revert filter back
            {
                setsockopt(m_dd, SOL_HCI, HCI_FILTER, &scan_filter_old, sizeof(scan_filter_old));
                scan_filter_old_valid = false;
            }

            if (m_scan_active)
            {
                m_scan_active = false;
                int err = hci_le_set_scan_enable(m_dd, 0x00, scan_filter_dup, 10000);
                if (err < 0) throw std::runtime_error("failed to disable scan");
            }
        }

    public:
        void asyncReadSome()
        {
            if (!m_read_active && m_stream.is_open())
            {
                m_read_active = true;
                boost::asio::async_read(m_stream, m_read_buf,
                    boost::asio::transfer_at_least(1),
                    boost::bind(&BluetoothDevice::onReadSome, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
            }
        }

        void readStop()
        {
            if (m_read_active && m_stream.is_open())
            {
                m_read_active = false;
                m_stream.cancel();
            }
        }

        json::Value getFoundDevices() const
        {
            json::Value res(json::Value::TYPE_OBJECT);

            std::map<String, String>::const_iterator i = m_scan_devices.begin();
            for (; i != m_scan_devices.end(); ++i)
            {
                const String &MAC = i->first;
                const String &name = i->second;

                res[MAC] = name;
            }

            return res;
        }

    private:
        bool m_scan_active;
        bool m_read_active;
        boost::asio::streambuf m_read_buf;
        std::map<String, String> m_scan_devices;

        void onReadSome(boost::system::error_code err, size_t len)
        {
            m_read_active = false;
            HIVE_UNUSED(len);

//            std::cerr << "read " << len << " bytes, err:" << err << "\n";
//            std::cerr << "dump:" << dumphex(m_read_buf.data()) << "\n";

            if (!err)
            {
                const uint8_t *buf = boost::asio::buffer_cast<const uint8_t*>(m_read_buf.data());
                size_t buf_len = m_read_buf.size();
                size_t len = buf_len;

                const uint8_t *ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
                len -= (1 + HCI_EVENT_HDR_SIZE);

                evt_le_meta_event *meta = (evt_le_meta_event*)ptr;
                if (meta->subevent == 0x02)
                {
                    le_advertising_info *info = (le_advertising_info *)(meta->data + 1);
                    // TODO: if (check_report_filter(filter_type, info))
                    {
                        char addr[64];
                        ba2str(&info->bdaddr, addr);
                        String name = parse_name(info->data, info->length);

                        m_scan_devices[addr] = name;
                    }
                }

                m_read_buf.consume(buf_len);
                asyncReadSome(); // continue
            }
        }


        template<typename Buf>
        static String dumphex(const Buf &buf)
        {
            return dump::hex(
                boost::asio::buffers_begin(buf),
                boost::asio::buffers_end(buf));
        }

        static String parse_name(uint8_t *eir, size_t eir_len)
        {
            #define EIR_NAME_SHORT              0x08  /* shortened local name */
            #define EIR_NAME_COMPLETE           0x09  /* complete local name */

            size_t offset;

            offset = 0;
            while (offset < eir_len)
            {
                uint8_t field_len = eir[0];
                size_t name_len;

                // Check for the end of EIR
                if (field_len == 0)
                    break;

                if (offset + field_len > eir_len)
                    break;

                switch (eir[1])
                {
                case EIR_NAME_SHORT:
                case EIR_NAME_COMPLETE:
                    name_len = field_len - 1;
                    return String((const char*)&eir[2], name_len);
                }

                offset += field_len + 1;
                eir += field_len + 1;
            }

            return "(unknown)";
        }

    public:


    public:

        /// @brief The "open" operation callback type.
        typedef boost::function1<void, boost::system::error_code> OpenCallback;


        /// @brief Is device open?
        bool is_open() const
        {
            return m_dd >= 0 && m_stream.is_open();
        }


        /// @brief Open device asynchronously.
        void async_open(OpenCallback callback)
        {
            boost::system::error_code err;

            if (!m_name.empty())
                m_dev_id = hci_devid(m_name.c_str());
            else
                m_dev_id = hci_get_route(NULL);

            if (m_dev_id >= 0)
            {
                if (hci_devba(m_dev_id, &m_dev_addr) >= 0)
                {
                    m_dd = hci_open_dev(m_dev_id);
                    if (m_dd >= 0)
                        m_stream.assign(m_dd);
                    else
                        err = boost::system::error_code(errno, boost::system::system_category());
                }
                else
                    err = boost::system::error_code(errno, boost::system::system_category());
            }
            else
                err = boost::system::error_code(errno, boost::system::system_category());

            m_ios.post(boost::bind(callback, err));
        }


        /// @brief Close device.
        void close()
        {
            m_stream.release();

            if (m_dd >= 0)
                hci_close_dev(m_dd);

            m_dd = -1;
            m_dev_id = -1;
            memset(&m_dev_addr, 0,
                sizeof(m_dev_addr));
        }

    private:
        boost::asio::io_service &m_ios;
        String m_name;

        bdaddr_t m_dev_addr; ///< @brief The BT address.
        int m_dev_id;        ///< @brief The device identifier.
        int m_dd;            ///< @brief The device descriptor.

        boost::asio::posix::stream_descriptor m_stream;
    };

    /// @brief The shared pointer type.
    typedef BluetoothDevice::SharedPtr BluetoothDevicePtr;

protected:

    /// @brief The default constructor.
    Application()
        : m_disableWebsockets(false)
        , m_disableWebsocketPingPong(false)
        , m_deviceRegistered(false)
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
        SharedPtr pthis(new This());

        String networkName = "C++ network";
        String networkKey = "";
        String networkDesc = "C++ device test network";

        String deviceId = "3305fe00-9bc9-11e4-bd06-0800200c9a66";
        String deviceName = "btle_gw";
        String deviceKey = "7adbc600-9bca-11e4-bd06-0800200c9a66";

        String baseUrl = "http://ecloud.dataart.com/ecapi8";
        size_t web_timeout = 0; // zero - don't change
        String http_version;

        String bluetoothName = "";

        // custom device properties
        for (int i = 1; i < argc; ++i) // skip executable name
        {
            if (boost::algorithm::iequals(argv[i], "--help"))
            {
                std::cout << argv[0] << " [options]";
                std::cout << "\t--gatewayId <gateway identifier>\n";
                std::cout << "\t--gatewayName <gateway name>\n";
                std::cout << "\t--gatewayKey <gateway authentication key>\n";
                std::cout << "\t--networkName <network name>\n";
                std::cout << "\t--networkKey <network authentication key>\n";
                std::cout << "\t--networkDesc <network description>\n";
                std::cout << "\t--server <server URL>\n";
                std::cout << "\t--web-timeout <timeout, seconds>\n";
                std::cout << "\t--http-version <major.minor HTTP version>\n";
                std::cout << "\t--no-ws disable automatic websocket service switching\n";
                std::cout << "\t--no-ws-ping-pong disable websocket ping/pong messages\n";
                std::cout << "\t--bluetooth <BTL device name or address>\n";

                exit(1);
            }
            else if (boost::algorithm::iequals(argv[i], "--gatewayId") && i+1 < argc)
                deviceId = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--gatewayName") && i+1 < argc)
                deviceName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--gatewayKey") && i+1 < argc)
                deviceKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkName") && i+1 < argc)
                networkName = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkKey") && i+1 < argc)
                networkKey = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--networkDesc") && i+1 < argc)
                networkDesc = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--server") && i+1 < argc)
                baseUrl = argv[++i];
            else if (boost::algorithm::iequals(argv[i], "--web-timeout") && i+1 < argc)
                web_timeout = boost::lexical_cast<UInt32>(argv[++i]);
            else if (boost::algorithm::iequals(argv[i], "--http-version") && i+1 < argc)
                http_version = argv[++i];
            else if (boost::iequals(argv[i], "--no-ws"))
                pthis->m_disableWebsockets = true;
            else if (boost::iequals(argv[i], "--no-ws-ping-pong"))
                pthis->m_disableWebsocketPingPong = true;
            else if (boost::algorithm::iequals(argv[i], "--bluetooth") && i+1 < argc)
                bluetoothName = argv[++i];
        }

        pthis->m_bluetooth = BluetoothDevice::create(pthis->m_ios, bluetoothName);
        pthis->m_network = devicehive::Network::create(networkName, networkKey, networkDesc);
        pthis->m_device = devicehive::Device::create(deviceId, deviceName, deviceKey,
            devicehive::Device::Class::create("BTLE gateway", "0.1", false, DEVICE_OFFLINE_TIMEOUT),
                                                     pthis->m_network);
        pthis->m_device->status = "Online";

        if (1) // create service
        {
            http::Url url(baseUrl);

            if (boost::iequals(url.getProtocol(), "ws")
                || boost::iequals(url.getProtocol(), "wss"))
            {
                if (pthis->m_disableWebsockets)
                    throw std::runtime_error("websockets are disabled by --no-ws switch");

                HIVELOG_INFO_STR(pthis->m_log, "WebSocket service is used");
                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                service->setPingPongEnabled(!pthis->m_disableWebsocketPingPong);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds

                pthis->m_service = service;
            }
            else
            {
                HIVELOG_INFO_STR(pthis->m_log, "RESTful service is used");
                devicehive::RestfulService::SharedPtr service = devicehive::RestfulService::create(
                    http::Client::create(pthis->m_ios), baseUrl, pthis);
                if (0 < web_timeout)
                    service->setTimeout(web_timeout*1000); // seconds -> milliseconds
                if (!http_version.empty())
                {
                    int major = 1, minor = 1;
                    parseVersion(http_version, major, minor);
                    service->setHttpVersion(major, minor);
                }

                pthis->m_service = service;
            }
        }

        return pthis;
    }


    /// @brief Get the shared pointer.
    /**
    @return The shared pointer to this instance.
    */
    SharedPtr shared_from_this()
    {
        return boost::dynamic_pointer_cast<This>(Base::shared_from_this());
    }

protected:

    /// @brief Start the application.
    /**
    Tries to open stream device.
    */
    virtual void start()
    {
        HIVELOG_TRACE_BLOCK(m_log, "start()");

        Base::start();
        m_service->asyncConnect();
        m_delayed->callLater( // ASAP
            boost::bind(&This::tryToOpenBluetoothDevice,
                shared_from_this()));
    }


    /// @brief Stop the application.
    /**
    Stops the "open" timer.
    */
    virtual void stop()
    {
        HIVELOG_TRACE_BLOCK(m_log, "stop()");

        m_service->cancelAll();
        if (m_bluetooth)
            m_bluetooth->close();

        Base::stop();
    }

private:

    /// @brief Try to open bluetooth device.
    /**
    */
    void tryToOpenBluetoothDevice()
    {
        if (m_bluetooth)
        {
            m_bluetooth->async_open(boost::bind(&This::onBluetoothDeviceOpen,
                shared_from_this(), _1));
        }
    }

    void onBluetoothDeviceOpen(boost::system::error_code err)
    {
        if (!err)
        {
            HIVELOG_INFO(m_log, "got bluetooth device OPEN: #"
                      << m_bluetooth->getDeviceId() << " "
                      << m_bluetooth->getDeviceAddressStr());
        }
        else
        {
            HIVELOG_DEBUG(m_log, "cannot open bluetooth device: ["
                << err << "] " << err.message());

            m_delayed->callLater(STREAM_RECONNECT_TIMEOUT,
                boost::bind(&This::tryToOpenBluetoothDevice,
                    shared_from_this()));
        }
    }



    /// @brief Reset the bluetooth device.
    /**
    @brief tryToReopen if `true` then try to reopen stream device as soon as possible.
    */
    virtual void resetBluetoothDevice(bool tryToReopen)
    {
        HIVELOG_WARN(m_log, "bluetooth device RESET");
        if (m_bluetooth)
            m_bluetooth->close();

        if (tryToReopen && !terminated())
        {
            m_delayed->callLater( // ASAP
                boost::bind(&This::tryToOpenBluetoothDevice,
                    shared_from_this()));
        }
    }

private: // IDeviceServiceEvents

    /// @copydoc devicehive::IDeviceServiceEvents::onConnected()
    virtual void onConnected(ErrorCode err)
    {
        HIVELOG_TRACE_BLOCK(m_log, "onConnected()");

        if (!err)
        {
            HIVELOG_DEBUG_STR(m_log, "connected to the server");
            m_service->asyncGetServerInfo();
        }
        else
            handleServiceError(err, "connection");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onServerInfo()
    virtual void onServerInfo(boost::system::error_code err, devicehive::ServerInfo info)
    {
        if (!err)
        {
            // don't update last command timestamp on reconnect
            if (m_lastCommandTimestamp.empty())
                m_lastCommandTimestamp = info.timestamp;

            // try to switch to websocket protocol
            if (!m_disableWebsockets && !info.alternativeUrl.empty())
                if (devicehive::RestfulService::SharedPtr rest = boost::dynamic_pointer_cast<devicehive::RestfulService>(m_service))
            {
                HIVELOG_INFO(m_log, "switching to Websocket service: " << info.alternativeUrl);
                rest->cancelAll();

                devicehive::WebsocketService::SharedPtr service = devicehive::WebsocketService::create(
                    rest->getHttpClient(), info.alternativeUrl, shared_from_this());
                service->setPingPongEnabled(!m_disableWebsocketPingPong);
                service->setTimeout(rest->getTimeout());
                m_service = service;

                // connect again as soon as possible
                m_delayed->callLater(boost::bind(&devicehive::IDeviceService::asyncConnect, m_service));
                return;
            }

            if (m_device)
                m_service->asyncRegisterDevice(m_device);
        }
        else
            handleServiceError(err, "getting server info");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onRegisterDevice()
    virtual void onRegisterDevice(boost::system::error_code err, devicehive::DevicePtr device)
    {
        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            m_deviceRegistered = true;

            m_service->asyncSubscribeForCommands(m_device, m_lastCommandTimestamp);
            sendPendingNotifications();
        }
        else
            handleServiceError(err, "registering device");
    }


    /// @copydoc devicehive::IDeviceServiceEvents::onInsertCommand()
    virtual void onInsertCommand(ErrorCode err, devicehive::DevicePtr device, devicehive::CommandPtr command)
    {
        if (m_device != device)     // if device's changed
            return;                 // just do nothing

        if (!err)
        {
            m_lastCommandTimestamp = command->timestamp;
            bool processed = true;

            try
            {
                processed = handleGatewayCommand(command);
            }
            catch (std::exception const& ex)
            {
                HIVELOG_ERROR(m_log, "handle command error: "
                    << ex.what());

                command->status = "Failed";
                command->result = ex.what();
            }

            if (processed)
                m_service->asyncUpdateCommand(device, command);
        }
        else
            handleServiceError(err, "polling command");
    }

private:

    /// @brief Handle gateway command.
    bool handleGatewayCommand(devicehive::CommandPtr command)
    {
        command->status = "Success";

        if (boost::iequals(command->name, "Hello"))
            command->result = "Good to see you!";
        else if (boost::iequals(command->name, "devices"))
            command->result = BluetoothDevice::getDevicesInfo();
        else if (boost::iequals(command->name, "info"))
        {
            if (!m_bluetooth || !m_bluetooth->is_open())
                throw std::runtime_error("No device");
            command->result = m_bluetooth->getDeviceInfo();
        }
        else if (boost::iequals(command->name, "exec/hciconfig"))
        {
            String cmd = "hciconfig ";
            cmd += command->params.asString();
            command->result = shellExec(cmd);
        }
        else if (boost::iequals(command->name, "exec/hcitool"))
        {
            String cmd = "hcitool ";
            cmd += command->params.asString();
            command->result = shellExec(cmd);
        }
        else if (boost::iequals(command->name, "exec/gatttool"))
        {
            String cmd = "gatttool ";
            cmd += command->params.asString();
            command->result = shellExec(cmd);
        }
        else if (boost::iequals(command->name, "scan/start")
              || boost::iequals(command->name, "scanStart")
              || boost::iequals(command->name, "startScan")
              || boost::iequals(command->name, "scan"))
        {
            if (!m_bluetooth || !m_bluetooth->is_open())
                throw std::runtime_error("No device");

            // force to update current pending scan command if any
            if (m_pendingScanCmdTimeout)
                m_pendingScanCmdTimeout->cancel();
            onScanCommandTimeout();

            m_bluetooth->scanStart(command->params);
            m_bluetooth->asyncReadSome();

            const int def_timeout = boost::iequals(command->name, "scan") ? 20 : 0;
            const int timeout = command->params.get("timeout", def_timeout).asUInt8(); // limited range [0..255]
            if (timeout != 0)
            {
                m_pendingScanCmdTimeout = m_delayed->callLater(timeout*1000,
                    boost::bind(&This::onScanCommandTimeout, shared_from_this()));
            }

            m_pendingScanCmd = command;
            return false; // pended
        }
        else if (boost::iequals(command->name, "scan/stop")
              || boost::iequals(command->name, "scanStop")
              || boost::iequals(command->name, "stopScan"))
        {
            if (!m_bluetooth || !m_bluetooth->is_open())
                throw std::runtime_error("No device");
            m_bluetooth->readStop();
            m_bluetooth->scanStop();

            // force to update current pending scan command if any
            if (m_pendingScanCmdTimeout)
                m_pendingScanCmdTimeout->cancel();
            onScanCommandTimeout();
        }
        else
            throw std::runtime_error("Unknown command");

        return true; // processed
    }


    /// @brief Send all pending notifications.
    void sendPendingNotifications()
    {
        const size_t N = m_pendingNotifications.size();
        HIVELOG_INFO(m_log, "sending " << N
            << " pending notifications");

        for (size_t i = 0; i < N; ++i)
        {
            m_service->asyncInsertNotification(m_device,
                m_pendingNotifications[i]);
        }
        m_pendingNotifications.clear();
    }


    /// @brief Send current scan results
    void onScanCommandTimeout()
    {
        if (m_service && m_bluetooth && m_pendingScanCmd)
        {
            m_pendingScanCmd->result = m_bluetooth->getFoundDevices();
            m_service->asyncUpdateCommand(m_device, m_pendingScanCmd);
            m_pendingScanCmd.reset();

            try
            {
                m_bluetooth->scanStop();
                m_bluetooth->readStop();
            }
            catch (const std::exception &ex)
            {
                HIVELOG_WARN(m_log, "ERROR stopping scan: " << ex.what());
            }
        }
    }

private:

    /// @brief Handle the service error.
    /**
    @param[in] err The error code.
    @param[in] hint The custom hint.
    */
    void handleServiceError(boost::system::error_code err, const char *hint)
    {
        if (!terminated())
        {
            HIVELOG_ERROR(m_log, (hint ? hint : "something")
                << " failed: [" << err << "] " << err.message());

            m_service->cancelAll();

            HIVELOG_DEBUG_STR(m_log, "try to connect later...");
            m_delayed->callLater(SERVER_RECONNECT_TIMEOUT,
                boost::bind(&devicehive::IDeviceService::asyncConnect, m_service));
        }
    }

private:
    BluetoothDevicePtr m_bluetooth;  ///< @brief The bluetooth device.

    devicehive::CommandPtr m_pendingScanCmd;
    basic_app::DelayedTask::SharedPtr m_pendingScanCmdTimeout;

private:
    devicehive::IDeviceServicePtr m_service; ///< @brief The cloud service.
    bool m_disableWebsockets;       ///< @brief No automatic websocket switch.
    bool m_disableWebsocketPingPong; ///< @brief Disable websocket PING/PONG messages.

private:
    devicehive::DevicePtr m_device; ///< @brief The device.
    devicehive::NetworkPtr m_network; ///< @brief The network.
    String m_lastCommandTimestamp; ///< @brief The timestamp of the last received command.
    bool m_deviceRegistered; ///< @brief The DeviceHive cloud "registered" flag.

private:
    // TODO: list of pending commands
    std::vector<devicehive::NotificationPtr> m_pendingNotifications; ///< @brief The list of pending notification.
};


/// @brief The BTLE gateway application entry point.
/**
Creates the Application instance and calls its Application::run() method.

@param[in] argc The number of command line arguments.
@param[in] argv The command line arguments.
*/
inline void main(int argc, const char* argv[])
{
    { // configure logging
        using namespace hive::log;

        Target::File::SharedPtr file = Target::File::create("btle_gw.log");
        file->setAutoFlushLevel(LEVEL_TRACE)
             .setMaxFileSize(1*1024*1024)
             .setNumberOfBackups(1)
             .startNew();

        Target::SharedPtr console = Logger::root().getTarget();
        console->setFormat(Format::create("%N: %M\n"))
                .setMinimumLevel(LEVEL_INFO);

        Logger::root().setTarget(Target::Tie::create(file, console))
                      .setLevel(LEVEL_TRACE);

        // disable annoying messages
        Logger("/hive/websocket").setTarget(file);
        Logger("/hive/http").setTarget(file);
    }

    Application::create(argc, argv)->run();
}

} // btle_gw namespace


///////////////////////////////////////////////////////////////////////////////
/** @page page_btle_gw BTLE gateway example

This page is under construction!

The libbluetooth-dev package should be installed.

@example examples/btle_gw.hpp
*/

#endif // __EXAMPLES_BTLE_GW_HPP_
