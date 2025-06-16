
#ifndef _TELLO_H
#define _TELLO_H


#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

// ====================================================================================
// ===                                                                            ===
// ===   Begin of the UDPsocket library (https://github.com/barczynsky/UDPsocket)   ===
// ===                                                                            ===
// ====================================================================================

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifndef INPORT_ANY
#define INPORT_ANY 0
#endif

#include <cstring>
#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <concepts>
#include <format>
#include <thread>
#include <chrono>
#include <mutex>
#include <functional>
#include <format>
#include <iostream>
#include <charconv>
#include <ranges>
#include <span>
#include <optional>



class UDPsocket
{
public:
    using sockaddr_in_t = struct sockaddr_in;
    using sockaddr_t    = struct sockaddr;
    using msg_t         = std::vector<uint8_t>;

public:
    struct IPv4;

    enum class Status : int
    {
        OK               = 0,
        SocketError      = -1,
        OpenError        = SocketError,
        CloseError       = -2,
        ShutdownError    = -3,
        BindError        = -4,
        ConnectError     = BindError,
        SetSockOptError  = -5,
        GetSockNameError = -6,
        SendError        = -7,
        RecvError        = -8,
    };

private:
    int             sock{ -1 };
    sockaddr_in_t   self_addr{};
    socklen_t       self_addr_len = sizeof(self_addr);
    sockaddr_in_t   peer_addr{};
    socklen_t       peer_addr_len = sizeof(peer_addr);

public:
    UDPsocket()
    {
#ifdef _WIN32
        WSAInit();
#endif
        self_addr = IPv4{};
        peer_addr = IPv4{};
    }

    ~UDPsocket()
    {
        close();
    }

public:
    int open()
    {
        close();
        sock = static_cast<int>(::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP));
        if (is_closed()) {
            return static_cast<int>(Status::SocketError);
        }
        return static_cast<int>(Status::OK);
    }

    int close()
    {
        if (!is_closed()) {
#ifdef _WIN32
            int ret = ::shutdown(sock, SD_BOTH);
#else
            int ret = ::shutdown(sock, SHUT_RDWR);
#endif
            if (ret < 0) {
                return static_cast<int>(Status::ShutdownError);
            }
#ifdef _WIN32
            ret = ::closesocket(sock);
#else
            ret = ::close(sock);
#endif
            if (ret < 0) {
                return static_cast<int>(Status::CloseError);
            }
            sock = -1;
        }
        return static_cast<int>(Status::OK);
    }

    bool is_closed() const { return sock < 0; }

public:
    int bind(const IPv4& ipaddr)
    {
        self_addr = ipaddr;
        self_addr_len = sizeof(self_addr);
        int opt = 1;
        int ret = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
        if (ret < 0) {
            return static_cast<int>(Status::SetSockOptError);
        }
        ret = ::bind(sock, reinterpret_cast<sockaddr_t*>(&self_addr), self_addr_len);
        if (ret < 0) {
            return static_cast<int>(Status::BindError);
        }
        ret = ::getsockname(sock, reinterpret_cast<sockaddr_t*>(&self_addr), &self_addr_len);
        if (ret < 0) {
            return static_cast<int>(Status::GetSockNameError);
        }
        return static_cast<int>(Status::OK);
    }

    int bind(uint16_t portno)
    {
        auto ipaddr = IPv4::Any(portno);
        return bind(ipaddr);
    }

    int bind_any()
    {
        return bind(INPORT_ANY);
    }

    int bind_any(uint16_t& portno)
    {
        int ret = bind(INPORT_ANY);
        if (ret < 0) {
            return ret;
        }
        portno = IPv4{ self_addr }.port;
        return static_cast<int>(Status::OK);
    }

public:
    int connect(const IPv4& ipaddr)
    {
        peer_addr = ipaddr;
        peer_addr_len = sizeof(peer_addr);
        int ret = ::connect(sock, reinterpret_cast<sockaddr_t*>(&peer_addr), peer_addr_len);
        if (ret < 0) {
            return static_cast<int>(Status::ConnectError);
        }
        return static_cast<int>(Status::OK);
    }

    int connect(uint16_t portno)
    {
        auto ipaddr = IPv4::Loopback(portno);
        return connect(ipaddr);
    }

public:
    IPv4 get_self_ip() const
    {
        return self_addr;
    }

    IPv4 get_peer_ip() const
    {
        return peer_addr;
    }

    int get_raw_socket() const
    {
        return sock;
    }

public:
    template<typename T>
    concept ByteContainer = sizeof(typename T::value_type) == sizeof(uint8_t);

    template <ByteContainer T>
    int send(const T& message, const IPv4& ipaddr) const
    {
        sockaddr_in_t addr_in = ipaddr;
        socklen_t addr_in_len = sizeof(addr_in);
        int ret = ::sendto(sock,
            reinterpret_cast<const char*>(message.data()), message.size(), 0,
            reinterpret_cast<sockaddr_t*>(&addr_in), addr_in_len);
        if (ret < 0) {
            return static_cast<int>(Status::SendError);
        }
        return ret;
    }

    template <ByteContainer T>
    int recv(T& message, IPv4& ipaddr) const
    {
        sockaddr_in_t addr_in;
        socklen_t addr_in_len = sizeof(addr_in);

        // Resize the vector to a buffer size suitable for UDP.
        // 1500 is a common MTU, 2048 is a safe choice.
        message.resize(2048);

        int ret = ::recvfrom(sock,
            reinterpret_cast<char*>(message.data()), message.size(), 0,
            reinterpret_cast<sockaddr_t*>(&addr_in), &addr_in_len);

        if (ret < 0) {
            message.clear(); // On error, ensure the message is empty.
            return static_cast<int>(Status::RecvError);
        }

        ipaddr = addr_in;
        message.resize(ret); // Truncate vector to the actual number of bytes received.
        return ret;
    }

public:
    int broadcast(int opt) const
    {
        int ret = ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&opt), sizeof(opt));
        if (ret < 0) {
            return static_cast<int>(Status::SetSockOptError);
        }
        return static_cast<int>(Status::OK);
    }

    int interrupt() const
    {
        uint16_t portno = IPv4{ self_addr }.port;
        auto ipaddr = IPv4::Loopback(portno);
        return send(msg_t{}, ipaddr);
    }

public:
    struct IPv4
    {
        std::array<uint8_t, 4> octets{};
        uint16_t port{};

    public:
        IPv4() = default;

        IPv4(std::string_view ipaddr, uint16_t portno)
        {
            // inet_pton requires a null-terminated string. A string_view may not be.
            // Create a null-terminated std::string to guarantee correctness.
            const std::string ip_str(ipaddr);
            if (::inet_pton(AF_INET, ip_str.c_str(), octets.data()) > 0) {
                port = portno;
            }
        }

        IPv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t portno)
            : octets{ a, b, c, d }, port(portno)
        {
        }

        IPv4(const sockaddr_in_t& addr_in)
        {
            *reinterpret_cast<uint32_t*>(octets.data()) = addr_in.sin_addr.s_addr;
            port = ntohs(addr_in.sin_port);
        }

        operator sockaddr_in_t() const
        {
            sockaddr_in_t addr_in;
            std::memset(&addr_in, 0, sizeof(addr_in));
            addr_in.sin_family = AF_INET;
            addr_in.sin_addr.s_addr = *reinterpret_cast<const uint32_t*>(octets.data());
            addr_in.sin_port = htons(port);
            return addr_in;
        }

    private:
        IPv4(uint32_t ipaddr, uint16_t portno)
        {
            *reinterpret_cast<uint32_t*>(octets.data()) = htonl(ipaddr);
            port = portno;
        }

    public:
        static IPv4 Any(uint16_t portno) { return { INADDR_ANY, portno }; }
        static IPv4 Loopback(uint16_t portno) { return { INADDR_LOOPBACK, portno }; }
        static IPv4 Broadcast(uint16_t portno) { return { INADDR_BROADCAST, portno }; }

    public:
        const uint8_t& operator[](size_t octet) const { return octets[octet]; }
        uint8_t&       operator[](size_t octet) { return octets[octet]; }

    public:
        auto operator<=>(const IPv4& other) const = default;

    public:
        std::string to_string() const {
            return std::format("{}.{}.{}.{}:{}", octets[0], octets[1], octets[2], octets[3], port);
        }

        operator std::string() const { return to_string(); }
    };

#ifdef _WIN32
public:
    static WSADATA* WSAInit()
    {
        static WSADATA wsa;
        static struct WSAContext {
            WSAContext(WSADATA* wsa_ptr) {
                WSAStartup(MAKEWORD(2, 2), wsa_ptr);
            }
            ~WSAContext() {
                WSACleanup();
            }
        } context{ &wsa };
        return &wsa;
    }
#endif
};

namespace std
{
    template<> struct hash<UDPsocket::IPv4>
    {
        using argument_type = UDPsocket::IPv4;
        using result_type = size_t;
        result_type operator()(const argument_type& ipaddr) const noexcept
        {
            result_type const h1{ std::hash<uint32_t>{}(*reinterpret_cast<const uint32_t*>(ipaddr.octets.data())) };
            result_type const h2{ std::hash<uint16_t>{}(ipaddr.port) };
            return h1 ^ (h2 << 1);
        }
    };
}

// =========================================
// ===                                   ===
// ===   End of the UDPsocket library    ===
// ===                                   ===
// =========================================


// =========================================
// ===                                   ===
// ===     Begin of Tello library        ===
// ===                                   ===
// =========================================
//
// Tello SDK 2.0:
// https://dl-cdn.ryzerobotics.com/downloads/Tello/Tello%20SDK%202.0%20User%20Guide.pdf
//



namespace TelloDefaults {
    static constexpr std::string_view IP = "192.168.10.1";
    static constexpr uint16_t COMMAND_PORT = 8889;
    static constexpr uint16_t DATA_PORT = 8890;
    static constexpr uint16_t LOCAL_PORT = 36085;
    static constexpr int COMMAND_TIMEOUT_MS = 1000;
    static constexpr int ACTION_TIMEOUT_MS = 0; // 0 = forever
}

// C++20 logging utilities using std::format and ANSI escape codes
enum class LogColor { Red, Green, Blue, Yellow, White };

void Log(LogColor color, std::string_view fmt, auto&&... args) {
    const char* color_code;
    switch (color) {
        case LogColor::Red:    color_code = "1;91"; break;
        case LogColor::Green:  color_code = "0;92"; break;
        case LogColor::Blue:   color_code = "1;94"; break;
        case LogColor::Yellow: color_code = "0;93"; break;
        default:               color_code = "0;97"; break;
    }
    std::cout << std::format("\033[{}m{}\033[m\n", color_code, std::vformat(fmt, std::make_format_args(args...)));
}

#define PRINTF_INFO(...) Log(LogColor::Green, __VA_ARGS__)
#define PRINTF_WARN(...) Log(LogColor::Yellow, __VA_ARGS__)
#define PRINTF_ERROR(...) Log(LogColor::Red, __VA_ARGS__)

#ifdef TELLO_DEBUG
#define PRINTF_DEBUG(...) Log(LogColor::Blue, __VA_ARGS__)
#else
#define PRINTF_DEBUG(...)
#endif


enum class FlipDirection : char {
    LEFT = 'l',
    RIGHT = 'r',
    FORWARD = 'f',
    BACK = 'b'
};

enum class MP_DetectDir {
    DOWNWARD_ONLY = 0,
    FORWARD_ONLY = 1,
    BOTH = 2
};

class Tello {

    class SyncSocket {
    public:
        SyncSocket(uint16_t sourcePort = 0) {
            if (socket.open() < 0) {
                PRINTF_ERROR("SyncSocket::SyncSocket: socket.open() failed.");
                return;
            }
            if (socket.bind(sourcePort) < 0) {
                PRINTF_ERROR("SyncSocket::SyncSocket: socket.bind() failed. Port {} may be in use.", sourcePort);
                return;
            }
        }

        bool send(std::string_view targetIP, uint16_t targetPort, std::string_view data) {
            UDPsocket::IPv4 ip(targetIP, targetPort);
            return socket.send(std::as_bytes(std::span(data)), ip) >= 0;
        }

        std::optional<std::string> recv(int timeout_ms = 0) {
            std::vector<uint8_t> buffer;
            UDPsocket::IPv4 sender;

            set_timeout(timeout_ms);
            if (socket.recv(buffer, sender) < 0)
                return std::nullopt;
            
            return std::string{reinterpret_cast<const char*>(buffer.data()), buffer.size()};
        }

    private:
        bool set_timeout(int timeout_ms) {
            if (timeout_ms != timeout) {
                timeout = timeout_ms;

#ifdef _WIN32
                DWORD _timeout = timeout_ms;
#else
                struct timeval _timeout;
                _timeout.tv_sec = timeout_ms / 1000;
                _timeout.tv_usec = (timeout_ms % 1000) * 1000;
#endif
                return ::setsockopt(socket.get_raw_socket(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&_timeout), sizeof(_timeout)) >= 0;
            }
            return true;
        }

    private:
        UDPsocket socket;
        int timeout = 0;
    };

    class AsyncSocket {
    public:
        AsyncSocket(uint16_t port, std::function<void(std::string_view)> cb) 
        : callback(std::move(cb)) {
            if (socket.open() < 0) {
                PRINTF_ERROR("AsyncSocket::AsyncSocket: socket.open() failed.");
                return;
            }
            if (socket.bind(port) < 0) {
                PRINTF_ERROR("AsyncSocket::AsyncSocket: socket.bind() failed. Port {} may be in use.", port);
                return;
            }
            listener = std::jthread([this](std::stop_token st) { listen(st); });
        }

        ~AsyncSocket() {
            listener.request_stop();
            socket.interrupt(); // Interrupt the blocking recv call
        }

        bool send(std::string_view ip, uint16_t port, std::string_view data) {
            UDPsocket::IPv4 _ip(ip, port);
            return socket.send(std::as_bytes(std::span(data)), _ip) >= 0;
        }

    private:
        void listen(std::stop_token st) {
            while (!st.stop_requested()) {
                std::vector<uint8_t> buffer;
                UDPsocket::IPv4 ipaddr;
                int error = socket.recv(buffer, ipaddr);

                if (st.stop_requested()) break;

                if (error < 0) {
                    PRINTF_ERROR("AsyncSocket::listen: socket.recv() failed: Error code {}", error);
                    continue;
                }

                if (callback)
                    callback({reinterpret_cast<const char*>(buffer.data()), buffer.size()});
            }
        }

    private:
        UDPsocket socket;
        std::jthread listener;
        std::function<void(std::string_view)> callback;
    };

    class MissionPadAPI {
    public:
        MissionPadAPI(Tello* tello) : tello(tello) {}

        bool enable_pad_detection() { return tello->execute_command("mon"); }
        bool disable_pad_detection() { return tello->execute_command("moff"); }
        bool set_pad_detection_direction(MP_DetectDir direction) {
            return tello->execute_command("mdirection {}", static_cast<int>(direction));
        }

        bool fly_straight_to_pad(float x, float y, float z, float speed, int mp_id) {
            return tello->execute_action("go {} {} {} {} m{}", x, y, z, speed, mp_id);
        }
        bool fly_arc_to_pad(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z, float speed_cmps, int mp_id) {
            return tello->execute_action("curve {} {} {} {} {} {} {} m{}", start_x, start_y, start_z, end_x, end_y, end_z, speed_cmps, mp_id);
        }
        bool jump_to_next_pad(float x, float y, float z, float speed, float yaw, int mp_id1, int mp_id2) {
            return tello->execute_action("jump {} {} {} {} {} m{} m{}", x, y, z, speed, yaw, mp_id1, mp_id2);
        }

    private:
        Tello* tello;
    };

public:

    struct TelloState {
        int32_t mp_id = 0, mp_x = 0, mp_y = 0, mp_z = 0;
        int32_t pitch = 0, roll = 0, yaw = 0;
        int32_t vgx = 0, vgy = 0, vgz = 0;
        int32_t templ = 0, temph = 0;
        uint32_t height = 0, h = 0, battery = 0;
        float sea_height = 0.f;
        int32_t time = 0;
        float agx = 0.f, agy = 0.f, agz = 0.f;
    };

public:
    Tello(
        uint16_t cmdPort = TelloDefaults::COMMAND_PORT,
        uint16_t dataPort = TelloDefaults::DATA_PORT,
        uint16_t locPort = TelloDefaults::LOCAL_PORT) :
        commandServer(locPort),
        dataServer(dataPort, [this](auto data) { OnDataStream(data); }),
        commandPort(cmdPort),
        missionPadAPI(this)
    {
    }

    ~Tello() {
        if (connected) {
            execute_action("land", true);
            execute_command("streamoff", true);
        }
    }

    bool connect(std::string_view ipAddress_sv = TelloDefaults::IP) {
        ipAddress = ipAddress_sv;
        PRINTF_INFO("[Tello] Connecting to {}", ipAddress);

        bool success = false;
        for (int i = 0; i < 10; ++i) {
            if (execute_command("command")) {
                success = true;
                break;
            }
            if (i < 9) { // Don't log warning or sleep on the last attempt
                 PRINTF_WARN("[Tello] Tello not found: Timeout. Retrying ({}/10)...", i + 1);
                 std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
        
        if (!success) {
            PRINTF_ERROR("[Tello] Failed to connect after 10 attempts. Please check the connection.");
            connected = false;
            return false;
        }

        connected = true;
        float battery = get_battery_level();
        PRINTF_INFO("[Tello] Connected: Battery level {:.0f}%", battery);

        if (battery < 5.f) {
            PRINTF_ERROR("[Tello] ERROR: The battery level is below 5%! Do not fly!");
            connected = false;
            return false;
        }
        else if (battery < 10.f) {
            PRINTF_WARN("[Tello] WARNING: The battery level is below 10%!");
        }
        return true;
    }

    // =============================================
    // ===                                       ===
    // ===   Implementation of the Tello SDK 2.0   ===
    // ===                                       ===
    // =============================================

    // === Control Commands ===
    bool takeoff() { return execute_action("takeoff"); }
    bool land() { return execute_action("land"); }
    bool enable_video_stream() { return execute_command("streamon"); }
    bool disable_video_stream() { return execute_command("streamoff"); }
    bool emergency() { return execute_command("emergency"); }

    bool move_up(float distance_cm) { return execute_action("up {}", distance_cm); }
    bool move_down(float distance_cm) { return execute_action("down {}", distance_cm); }
    bool move_left(float distance_cm) { return execute_action("left {}", distance_cm); }
    bool move_right(float distance_cm) { return execute_action("right {}", distance_cm); }
    bool move_forward(float distance_cm) { return execute_action("forward {}", distance_cm); }
    bool move_back(float distance_cm) { return execute_action("back {}", distance_cm); }

    bool turn_right(float angle_deg) { return execute_action("cw {}", angle_deg); }
    bool turn_left(float angle_deg) { return execute_action("ccw {}", angle_deg); }

    bool flip(FlipDirection flipDirection) {
        return execute_action("flip {}", static_cast<char>(flipDirection));
    }

    bool move_by(float x, float y, float z, float speed_cmps) { return execute_action("go {} {} {} {}", x, y, z, speed_cmps); }
    bool stop() { return execute_command("stop"); }

    bool fly_arc(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z, float speed_cmps) {
        return execute_action("curve {} {} {} {} {} {} {}", start_x, start_y, start_z, end_x, end_y, end_z, speed_cmps);
    }

    // === Set Commands ===
    bool set_speed(float speed) { return execute_command("speed {}", speed); }
    bool move(float left_right, float forward_back, float up_down, float yaw) {
        return execute_command("rc {} {} {} {}", left_right, forward_back, up_down, yaw);
    }
    bool set_wifi_password(std::string_view ssid, std::string_view password) {
        return execute_command("wifi {} {}", ssid, password);
    }
    bool connect_to_wifi(std::string_view ssid, std::string_view password) {
        return execute_action("ap {} {}", ssid, password);
    }

    // === Read Commands ===
    float get_speed() { return get_float("speed?"); }
    float get_battery_level() { return get_float("battery?"); }
    std::string get_flight_time() { return get_str("time?"); }
    std::string get_wifi_snr() { return get_str("wifi?"); }
    std::string get_sdk_version() { return get_str("sdk?"); }
    std::string get_serial_number() { return get_str("sn?"); }

    MissionPadAPI missionPadAPI;

    bool execute_manual_command(std::string_view command, int timeout_ms) {
        return execute_command_raw(command, timeout_ms);
    }

    std::string get_manual_response(std::string_view command) {
        return get_str(command);
    }

    bool is_connected() const {
        return connected;
    }

    void sleep(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void set_action_timeout(int timeout_ms) {
        actionTimeout = timeout_ms;
    }

    void set_command_timeout(int timeout_ms) {
        commandTimeout = timeout_ms;
    }

    TelloState state() {
        std::lock_guard lock(stateMTX);
        return _state;
    }

private:
    template<typename T>
    T parse_value(std::string_view sv) {
        T value{};
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);
        if (result.ec == std::errc()) {
            return value;
        }
        return T{};
    }

    float get_float(std::string_view cmd) {
        std::string response = get_str(cmd);
        if (response.empty()) return 0.f;
        return parse_value<float>(response);
    }

    template<typename... TArgs>
    bool execute_command(std::string_view fmt, TArgs&&... args) {
        return execute_command_raw(std::vformat(fmt, std::make_format_args(args...)), commandTimeout);
    }
    
    template<typename... TArgs>
    bool execute_action(std::string_view fmt, TArgs&&... args) {
        return execute_command_raw(std::vformat(fmt, std::make_format_args(args...)), actionTimeout);
    }

    std::string get_str(std::string_view str, bool silent = false) {
        auto response = send_request(str, commandTimeout, silent);
        return response.value_or("");
    }

    bool execute_command_raw(std::string_view str, int timeout_ms, bool silent = false) {
        auto response = send_request(str, timeout_ms, silent);
        if (!response.has_value())
            return false;

        if (response.value() != "ok") {
            if (!silent) PRINTF_ERROR("[Tello] Failed to send command '{}': Expected 'ok', received '{}'", str, response.value());
            return false;
        }
        return true;
    }

    std::optional<std::string> send_request(std::string_view str, int timeout_ms, bool silent) {
        if (!connected) {
            if (!silent) PRINTF_ERROR("[Tello] Tello not connected");
            return std::nullopt;
        }

        if (!silent) PRINTF_DEBUG("[Tello] DEBUG: Sending command '{}'", str);

        std::unique_lock lock(requestMTX);
        if (!commandServer.send(ipAddress, commandPort, str)) {
            if (!silent) PRINTF_ERROR("[Tello] Failed to send command '{}': Socket error", str);
            return std::nullopt;
        }

        auto response = commandServer.recv(timeout_ms);
        if (!response.has_value()) {
            if (!silent) PRINTF_ERROR("[Tello] Failed to send command '{}': Timeout waiting for response", str);
            return std::nullopt;
        }

        return response;
    }

    void OnDataStream(std::string_view data) {
        std::lock_guard lock(stateMTX);
        for (const auto token_range : data | std::views::split(';')) {
            std::string_view token(token_range.begin(), token_range.end());
            if (token.empty()) continue;

            auto pos = token.find(':');
            if (pos == std::string_view::npos) continue;

            auto key = token.substr(0, pos);
            auto value = token.substr(pos + 1);

            if (key == "mid")      _state.mp_id = parse_value<int32_t>(value);
            else if (key == "x")   _state.mp_x = parse_value<int32_t>(value);
            else if (key == "y")   _state.mp_y = parse_value<int32_t>(value);
            else if (key == "z")   _state.mp_z = parse_value<int32_t>(value);
            else if (key == "pitch") _state.pitch = parse_value<int32_t>(value);
            else if (key == "roll")  _state.roll = parse_value<int32_t>(value);
            else if (key == "yaw")   _state.yaw = parse_value<int32_t>(value);
            else if (key == "vgx")  _state.vgx = parse_value<int32_t>(value);
            else if (key == "vgy")  _state.vgy = parse_value<int32_t>(value);
            else if (key == "vgz")  _state.vgz = parse_value<int32_t>(value);
            else if (key == "templ") _state.templ = parse_value<int32_t>(value);
            else if (key == "temph") _state.temph = parse_value<int32_t>(value);
            else if (key == "tof")   _state.height = parse_value<uint32_t>(value);
            else if (key == "h")     _state.h = parse_value<uint32_t>(value);
            else if (key == "bat")   _state.battery = parse_value<uint32_t>(value);
            else if (key == "baro")  _state.sea_height = parse_value<float>(value);
            else if (key == "time")  _state.time = parse_value<int32_t>(value);
            else if (key == "agx")   _state.agx = parse_value<float>(value);
            else if (key == "agy")   _state.agy = parse_value<float>(value);
            else if (key == "agz")   _state.agz = parse_value<float>(value);
        }
    }

private:
    SyncSocket commandServer;
    AsyncSocket dataServer;

    bool connected = false;

    std::string ipAddress;
    uint16_t commandPort = 0;
    int commandTimeout = TelloDefaults::COMMAND_TIMEOUT_MS;
    int actionTimeout = TelloDefaults::ACTION_TIMEOUT_MS;

    std::mutex requestMTX;
    std::mutex stateMTX;
    TelloState _state;
};

#endif // _TELLO_H