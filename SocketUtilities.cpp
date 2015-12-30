#include "SocketUtilities.h"
#include <limits>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <sstream>
#include <string>
#include <sys/socket.h>

// MSG_NOSIGNAL doesn't exist on Mac OSX
#if defined(__APPLE__)
    #define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

class SocketUtilities::SocketException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
    
/*
 * Redefine standard aliases and alias the STL types used
 */
using SocketType = SocketUtilities::SocketType;
using SocketException = SocketUtilities::SocketException;
using BufferType = SocketUtilities::BufferType;
using std::ostringstream;
using std::cout;
using std::endl;
using std::ostream;
using std::string;
using std::to_string;

/*
 * The standard stream to which logged messages are displayed.
 */
static ostream* log_stream = &std::cout;

/*
 * Defaulted compile time logging utility for this class.  Optimized away at
 * compile time when the network log is not required
 */
static std::atomic<bool> __log_mutex (false);
template <bool output> void _log_output(const string& output_message);
template <> void _log_output<true> (const string& output_message) {

    // spin and acquire the lock
    while (__log_mutex.exchange(false)) {}
        *log_stream << "@@@ Network Log @@@  " << output_message << 
            (output_message.back() == '\n' ? "" : "\n");
    __log_mutex.store(false);
}
template <> void _log_output<false> (__attribute__((unused)) // silence unused 
        const string& output_message) {}                     // variable warning
static constexpr void (*log_output) (const string& output_message) = 
    _log_output<log_events>;


/******************************************************************************
 ************************* FUNCTION IMPLEMENTIONS *****************************
 ******************************************************************************/
void SocketUtilities::set_log_stream(std::ostream& log_stream_in) {
    log_stream = &log_stream_in;
}

SocketType SocketUtilities::create_server_socket(const char* port, int backlog) {

    SocketType socket_to_return;

    // ****************************** STEP 1 **********************************
    // ************************************************************************
    // Create the addrinfo struct to get the information needed to bind
    addrinfo hints, *server_address_information;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;    // TCP socket
    hints.ai_flags = AI_PASSIVE;        // Automatically fill in my IP

    // getaddrinfo to get the results of the internet address on which to bind
    // the socket connection
    int return_value;
    if ((return_value = 
          getaddrinfo(nullptr, port, &hints, &server_address_information)) != 0) {
        throw SocketException( (ostringstream() << "getaddrinfo: " << 
                    gai_strerror(return_value)).str() );
    }

    // loop through the results from getaddrinfo
    { addrinfo* i;
    for (i = server_address_information; i; i = i->ai_next) {

        // continue if this fails
        if ((socket_to_return = socket(i->ai_family, i->ai_socktype,
                i->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // exit if setsockopt fails
        int yes = 1;
        if (setsockopt(socket_to_return, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            throw SocketException("Error in setsockopt");
        }

        // continue trying if this fails
        if (::bind(socket_to_return, i->ai_addr, i->ai_addrlen) == -1) {
            close(socket_to_return);
            perror("server: bind");
            continue;
        }

        break;
    }
    freeaddrinfo(server_address_information); 
    if (!i) {
        throw SocketException("Failed to bind to this machine's IP and "
                "port specified");
    } }
    // ************************************************************************
    // ****************************** STEP 1 **********************************
    

    // ****************************** STEP 2 **********************************
    // ************************************************************************
    if (listen(socket_to_return, backlog) == -1) {
        perror("listen");
        throw SocketException("error in listen()");
    }

    // log output
    log_output(string("Created server socket ") + to_string(socket_to_return));

    return socket_to_return;
}

auto SocketUtilities::recv(SocketType sock_fd, void* buffer, 
        size_t length, int flags) -> decltype (::recv(0,0,0,0)) {


    // Make the system call and handle errors
    auto n = ::recv(sock_fd, buffer, length, flags);
    if (n == -1) {
        throw SocketUtilities::SocketException(
                string("Error calling recv() : ") + string (strerror(errno)));
    } else if (!n) {
        throw SocketUtilities::SocketException(
                string("Error calling recv() : ") + string (strerror(errno)));
    }

    // Print to the network log
    log_output(string("Called recv() on socket ") + to_string(sock_fd) + 
            string(" : ") + string("received ") + to_string(n) + 
            string(" bytes\n") + string((char*)buffer, ((char*)buffer) + n));

    return n;
}


BufferType SocketUtilities::receive (SocketType sock_fd, 
        size_t length, unsigned flags) {

    auto bytes_received = decltype(length)(0);
    BufferType buffer (length);

    // loop and receive data
    while (bytes_received < length) {
        int received = SocketUtilities::recv(sock_fd, 
                (void*) (buffer.data() + bytes_received), 
                length - bytes_received, 
                flags);
        bytes_received += received;
    }

    return buffer;
}

void SocketUtilities::send(SocketType sock_fd, BufferType data_to_send) {
    
    // loop and send data 
    size_t bytes_sent = 0;
    while (bytes_sent < data_to_send.size()) {
        int sent = ::send(sock_fd, data_to_send.data() + bytes_sent, 
                data_to_send.size() - bytes_sent, MSG_NOSIGNAL);
        
        // handle exceptional condition
        if (sent == -1) {
            throw SocketException(string("Error sending data to socket ") + 
                    to_string(sock_fd));
        }

        // print to log if logging is turned on
        log_output(string("Called send() on socket ") + to_string(sock_fd) + 
                string(" : sent ") + to_string(sent) + string(" bytes\n") + 
                string(data_to_send.data() + bytes_sent, 
                    data_to_send.data() + bytes_sent + sent));

        bytes_sent += sent;
    }

}

SocketType SocketUtilities::accept(SocketType sock_fd, 
        sockaddr* address, socklen_t* address_length) {

    auto to_return_socket = ::accept(sock_fd, address, address_length);
    if (to_return_socket == -1) {
        perror("accept");
        throw SocketException("Error calling accept()");
    }

    log_output(string("Accepted new connection on socket ") + 
            to_string(to_return_socket));
    return to_return_socket;
}
/******************************************************************************
 ************************* FUNCTION IMPLEMENTIONS *****************************
 ******************************************************************************/


/******************************************************************************
 *********************** The RAII wrapper for sockets *************************
 ******************************************************************************/
using SocketRAII = SocketUtilities::SocketRAII;
const SocketType SocketRAII::null_socket = std::numeric_limits<SocketType>::max();

void SocketRAII::release() {
    if (this->owned_socket != SocketRAII::null_socket) {
        close(this->owned_socket); 
        this->owned_socket = SocketRAII::null_socket; 
    }
}

SocketRAII::SocketRAII(int sock_fd) : owned_socket(sock_fd) {}
SocketRAII::~SocketRAII() { this->release(); }
SocketRAII::SocketRAII(SocketRAII&& other) {
    this->owned_socket = other.owned_socket;
    other.owned_socket = SocketRAII::null_socket;
}

SocketRAII::operator SocketType () { return this->owned_socket; }
/******************************************************************************
 *********************** The RAII wrapper for sockets *************************
 ******************************************************************************/



/*
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
*/
