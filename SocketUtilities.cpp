#include "SocketUtilities.hpp"
#include <cassert>
#include <limits>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

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
std::atomic<bool> network_output_protect (false);
template <bool output> void _log_output(const string& output_message);
template <> void _log_output<true> (const string& output_message) {

    // spin and acquire the lock
    while (network_output_protect.exchange(false)) {}
        *log_stream << "@@@ Network Log @@@  " << output_message << 
            (output_message.back() == '\n' ? "" : "\n");
    network_output_protect.store(false);
}
template <> void _log_output<false> (__attribute__((unused)) // silence unused 
        const string& output_message) {}                     // variable warning
static constexpr void (*log_output) (const string& output_message) = 
    _log_output<log_events>;


/******************************************************************************
 *                           FUNCTION IMPLEMENTIONS                           *
 ******************************************************************************/
void SocketUtilities::set_log_stream(std::ostream& log_stream_in) {
    log_stream = &log_stream_in;
}

SocketType SocketUtilities::create_server_socket(const char* port, int backlog) {

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
    // *                               /STEP 1                                *
    // ************************************************************************
    


    // ************************************************************************
    // *                               STEP 2                                 *
    // ************************************************************************
    if (listen(socket_to_return, backlog) == -1) {
        perror("listen");
        throw SocketException("error in listen()");
    }
    // ************************************************************************
    // *                              /STEP 2                                 *
    // ************************************************************************

    // log output
    log_output(string("Created server socket ") + to_string(socket_to_return));

    return socket_to_return;
}

auto SocketUtilities::recv (SocketType sock_fd, void* buffer, 
        size_t length, int flags) -> decltype (SocketUtilities::recv(0,0,0,0)) {

    // Make the system call and handle errors
    auto n = ::recv(sock_fd, buffer, length, flags);

    // Exceptional conditions are not tolerated for stream sockets.  In the case
    // of a non blocking socket, if there is a potential blocking situation then
    // the user did not call poll() correctly and did not perform the poll()
    // operation correctly, either using the normal system call or the wrapper
    // provided in this module. 
    if ( (!n) || (n == -1) ) {
        throw SocketException(string("recv() on socket ") + 
                to_string(sock_fd) + string(" returned with error ") + 
                string(strerror(errno)));

    }

    // Print to the network log
    log_output(string("Called recv() on socket ") + to_string(sock_fd) + 
            string(" : ") + string("received ") + to_string(n) + 
            string(" bytes\n") + string((char*)buffer, ((char*)buffer) + n));

    return n;
}

auto SocketUtilities::send (SocketType sock_fd, void* buffer, size_t length, 
        int flags) -> decltype(SocketUtilities::send(0,0,0,0)) {

    auto n = ::send(sock_fd, buffer, length, flags);

    // Handle exceptional conditions, keep in mind that a return value of 0 is
    // not an exceptional condition.  This is in fact a feature of TCP;  this
    // may indicate that the other side is not keeping up.  If the other side
    // has ended the connection then SIGPIPE would be generated if the flags do
    // not include MSG_NOSIGNAL
    if (n == -1) {
        throw SocketException(string("send() on socket ") + 
                to_string(sock_fd) + string(" returned with error ") + 
                string(strerror(errno)));
    }
    
    // Print to the network log
    log_output(string("Called send() on socket ") + to_string(sock_fd) + 
            string(" : ") + string("sent ") + to_string(n) + 
            string(" bytes\n") + string((char*)buffer, ((char*)buffer) + n));

    return n;
}

void SocketUtilities::send_all (SocketType sock_fd, 
        const BufferType& data_to_send) {
    
    // loop and send data 
    auto bytes_sent = decltype(data_to_send.size()) {0};
    while (bytes_sent < data_to_send.size()) {
        int sent = SocketUtilities::send(sock_fd, 
                (void*) (data_to_send.data() + bytes_sent), 
                data_to_send.size() - bytes_sent, MSG_NOSIGNAL);
        bytes_sent += sent;

        // assert that too many bytes have not been sent.  I do not know if this
        // can happen but just doing it here because it's my responsibility to
        // keep this code safe.
        assert(bytes_sent <= data_to_send.size());
    }

}

auto SocketUtilities::accept(SocketType sock_fd, sockaddr* address, 
        socklen_t* address_length) -> decltype(SocketUtilities::accept(0,0,0)) {

    auto to_return_socket = ::accept(sock_fd, address, address_length);
    if (to_return_socket == -1) {
        throw SocketException(string("Error calling accept() on socket ") + 
                to_string(sock_fd) + string(" : ") + 
                string(strerror(errno)));
    }

    // assert that something horribly wrong didn't happen.  stdout, stdin and
    // stderr are protected members of the file descriptor family.
    assert(to_return_socket != STDOUT_FILENO && 
           to_return_socket != STDIN_FILENO && 
           to_return_socket != STDERR_FILENO);

    log_output(string("Accepted new connection on socket ") + 
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
