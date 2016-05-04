#include "SocketUtilities.hpp"
#include <cassert>
#include <limits>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <limits>

using namespace std::literals::string_literals;

// MSG_NOSIGNAL doesn't exist on Mac OSX
#if defined(__APPLE__)
    #define MSG_NOSIGNAL SO_NOSIGPIPE
#endif

/* The default exception class, nothing more than a simple runtime_error */
class SocketUtilities::SocketException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
    
/* Redefine standard aliases and alias the STL types used */
using SocketUtilities::SocketType;
using SocketUtilities::SocketException;
using std::ostringstream;
using std::cout;
using std::cerr;
using std::endl;
using std::ostream;
using std::string;
using std::to_string;
using std::vector;
using namespace std::literals::string_literals; /* for operator "" */

/* The standard stream to which logged messages are displayed. */
static ostream* log_stream = &std::cout;

/* define the global network_output_protect variable */
namespace SocketUtilities { 
    std::atomic<bool> network_output_protect (false); 
}
using SocketUtilities::network_output_protect;

/*
 * Logging utility for this class.  Optimized away at compile time when the
 * network log is not required.  One can enable log messages by defining the
 * macro SOCKET_LOG_COMMUNICATION when building.
 */
template <bool output> void _log_output(const string& output_message);
template <> void _log_output<true>(const string& output_message) {

    // spin and acquire the lock
    while (network_output_protect.exchange(true)) {}
        (*log_stream) << "@@@ Network Log @@@  " << output_message << 
            (output_message.back() == '\n' ? "" : "\n");
    network_output_protect.store(false);
}
template <> void _log_output<false> (__attribute__((unused)) // silence unused 
        const string& output_message) {}                     // variable warning
static constexpr void (*log_output) (const string& output_message) = 
    _log_output<SocketUtilities::log_events>;


/******************************************************************************
 *                           FUNCTION IMPLEMENTIONS                           *
 ******************************************************************************/
void SocketUtilities::set_log_stream(std::ostream& log_stream_in) {

    // spin and acquire lock
    while(network_output_protect.exchange(true)) {}
        log_stream = &log_stream_in;
    network_output_protect.store(false);
}

SocketType SocketUtilities::create_server_socket(const char* port, 
        int backlog) {

    SocketType socket_to_return;

    // ************************************************************************
    // *                                STEP 1                                *
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
        throw SocketException("getaddrinfo: "s + 
                string(gai_strerror(return_value)));
    }

    // loop through the results from getaddrinfo
    addrinfo* i;
    for (i = server_address_information; i; i = i->ai_next) {

        // ********************************************************************
        // *                            STEP 2                                *
        // ********************************************************************
        // continue if this fails
        if ((socket_to_return = socket(i->ai_family, i->ai_socktype,
                i->ai_protocol)) == -1) {
            cerr << "Error creating server socket " << strerror(errno) << endl;
            continue;
        }

        // ********************************************************************
        // *                            STEP 3                                *
        // ********************************************************************
        // exit if setsockopt fails
        int yes = 1;
        if (setsockopt(socket_to_return, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            cerr << "Error calling setsockopt on server socket " 
                << strerror(errno) << endl;
            throw SocketException("Error in setsockopt");
        }

        // ********************************************************************
        // *                            STEP 4                                *
        // ********************************************************************
        // continue trying if this fails
        if (::bind(socket_to_return, i->ai_addr, i->ai_addrlen) == -1) {
            close(socket_to_return);
            cerr << "Error calling bind on server socket " << strerror(errno) 
                << endl;
            continue;
        }

        break;
    }
    freeaddrinfo(server_address_information); 
    if (!i) {
        throw SocketException("Failed to bind to this machine's IP and "
                "port specified");
    }

    // ************************************************************************
    // *                               STEP 5                                 *
    // ************************************************************************
    if (listen(socket_to_return, backlog) == -1) {
        perror("listen");
        throw SocketException("error in listen()");
    }

    // log output
    log_output("Created server socket "s + to_string(socket_to_return));

    return socket_to_return;
}

SocketType SocketUtilities::create_client_socket(const char* address, 
        const char* port) {
    
    SocketType socket_to_return;

    // ************************************************************************
    // *                                STEP 1                                *
    // ************************************************************************
    // Create the addrinfo struct to get the information needed to connect to
    // the remote host
    addrinfo hints, *server_address_information;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;        // ipv4 or ipv6
    hints.ai_socktype = SOCK_STREAM;    // TCP socket

    // make call to getaddrinfo
    int return_value;
    if ((return_value = getaddrinfo(address, port, &hints, 
                    &server_address_information)) != 0) {

        throw SocketException("getaddrinfo: "s + 
                string(gai_strerror(return_value)));
    } 

    addrinfo* i = nullptr;
    for (i = server_address_information; i; i = i->ai_next) {
        
        // ********************************************************************
        // *                            STEP 2                                *
        // ********************************************************************
        // attempt to create a socket with the address info structure returned
        // by the getaddrinfo call
        if ((socket_to_return = socket(i->ai_family, i->ai_socktype, 
                    i->ai_protocol)) == -1)  {
            cerr << "Error on creating client socket " << strerror(errno) << endl;
            continue;
        }

        // ********************************************************************
        // *                            STEP 3                                *
        // ********************************************************************
        // attempt to connect to the remote server
        if (connect(socket_to_return, i->ai_addr, i->ai_addrlen) == -1) {
            close(socket_to_return);
            cerr << "Error connecting to remote server " << strerror(errno) 
                << endl;
            continue;
        }

        break;
    }

    if (!i) {
        throw SocketException("Failed to connect to remote server");

    }

    // all done with this structure
    freeaddrinfo(server_address_information); 

    // return the socket
    return socket_to_return;
}

SocketType SocketUtilities::create_server_unix_socket(
        const string& socket_path, int backlog) {
    
    // STEP 1 : socket()
    int unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        throw SocketException {"Error occured in socket() call : "s + 
            string(std::strerror(errno))};
    }

    // STEP 2 : bind(), setup the address structures for bind()
    sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;
    std::copy(socket_path.begin(), socket_path.end(), 
            local_address.sun_path);

    // unlink from before
    ::unlink(socket_path.c_str());

    // STEP 2 : bind()
    if (::bind(unix_socket, reinterpret_cast<sockaddr*>(&local_address), 
                sizeof(local_address)) == -1) {
        throw SocketException {"Error occured in bind() call : "s + 
            string(strerror(errno))};
    }

    // STEP 3 : listen()
    if (::listen(unix_socket, backlog) == -1) {
        throw SocketException {"Error occured in listen() call : "s + 
            string(strerror(errno))};
    }

    // log output
    log_output("Created unix server socket on file descriptor"s + 
            to_string(unix_socket) + "connected to file "s + socket_path);

    return unix_socket;

}

SocketType SocketUtilities::create_client_unix_socket(
        const std::string& socket_path) {

    // STEP 1 : socket()
    int unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        throw SocketException {"Error in socket() call : "s + 
            string(strerror(errno))};
    }
    
    // STEP 2 : connect()
    sockaddr_un remote_address;
    remote_address.sun_family = AF_UNIX;
    std::copy(socket_path.begin(), socket_path.end(), 
            remote_address.sun_path);

    if (::connect(unix_socket, 
                reinterpret_cast<sockaddr*>(&remote_address), 
                sizeof(remote_address)) == -1) {
        throw SocketException {"Error in connect() call : "s + 
            string(strerror(errno))};
    }

    // log output
    log_output("Created unix client socket on file descriptor "s + 
            to_string(unix_socket) + " connected to file "s + socket_path);

    return unix_socket;
}

ssize_t SocketUtilities::recv(SocketType sock_fd, void* buffer,
        size_t length, int flags) {

    // Make the system call and handle errors
    ssize_t n = ::recv(sock_fd, buffer, length, flags);

    // Exceptional conditions are not tolerated for stream sockets.  In the
    // case of a non blocking socket, if there is a potential blocking
    // situation then the user did not call poll() correctly and did not
    // perform the poll() operation correctly, either using the normal system
    // call or the wrapper provided in this module.
    //
    // If a threadsafe genralized version of this call is used either in the
    // form of kqueues or epoll then the user should poll the socket file
    // descriptor such that the recv function is not called on a socket that
    // is non blocking when there isnt any data in the socket to read from.
    if (n == -1) {
        throw SocketException("recv() on socket "s + 
                to_string(sock_fd) + " returned with error "s + 
                string(strerror(errno)));

    }

    // Print to the network log
    log_output("Called recv() on socket "s + to_string(sock_fd) + " : "s + 
            "received "s + to_string(n) + " bytes\n"s + 
            string((char*)buffer, ((char*)buffer) + n));

    return n;
}

ssize_t SocketUtilities::send(SocketType sock_fd, const void* buffer, 
        size_t length, int flags) {

    ssize_t n = ::send(sock_fd, const_cast<void*>(buffer), length, flags);

    // Handle exceptional conditions, keep in mind that a return value of 0 is
    // not an exceptional condition.  This is in fact a feature of TCP;  this
    // may indicate that the other side is not keeping up.  If the other side
    // has ended the connection then SIGPIPE would be generated if the flags do
    // not include MSG_NOSIGNAL
    if (n == -1) {
        throw SocketException("send() on socket "s + 
                to_string(sock_fd) + " returned with error "s + 
                string(strerror(errno)));
    }
    
    // Print to the network log
    log_output("Called send() on socket "s + to_string(sock_fd) + " : "s + 
            "sent "s + to_string(n) + " bytes\n"s + 
            string(reinterpret_cast<const char*>(buffer), 
                reinterpret_cast<const char*>(buffer) + n));

    return n;
}

void SocketUtilities::send_all(SocketType sock_fd, const void* buffer, 
        size_t length) {

    assert(length <= std::numeric_limits<int>::max());

    // loop and send data
    int bytes_sent {0};
    while (bytes_sent < static_cast<int>(length)) {

        // send the bytes and keep track of how many have been sent until now
        int sent = SocketUtilities::send(sock_fd, 
                reinterpret_cast<const void*>(
                    reinterpret_cast<const char*>(buffer) + bytes_sent),
                length - bytes_sent, MSG_NOSIGNAL);
        bytes_sent += sent;

        // assert that too many bytes have not been sent.
        assert(bytes_sent <= static_cast<int>(length));
    }
}

void SocketUtilities::send_all(SocketType sock_fd, 
        const vector<char>& data_to_send) {

    // forward to another sending function
    SocketUtilities::send_all(sock_fd, 
            reinterpret_cast<const void*>(data_to_send.data()),
            data_to_send.size());
}

SocketType SocketUtilities::accept(SocketType sock_fd, sockaddr* address, 
        socklen_t* address_length) {

    SocketType to_return_socket = ::accept(sock_fd, address, address_length);
    if (to_return_socket == -1) {
        throw SocketException("Error calling accept() on socket "s + 
                to_string(sock_fd) + " : "s + string(strerror(errno)));
    }

    // assert that something horribly wrong didn't happen.  stdout, stdin and
    // stderr are protected members of the file descriptor family.
    assert(to_return_socket != STDOUT_FILENO && 
           to_return_socket != STDIN_FILENO && 
           to_return_socket != STDERR_FILENO);

    log_output("Accepted new connection on socket "s + 
            to_string(to_return_socket));
    return to_return_socket;
}
/******************************************************************************
 *                          /FUNCTION IMPLEMENTIONS                           *
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
