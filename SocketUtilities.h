#ifndef __SOCKET_UTILITIES__
#define __SOCKET_UTILITIES__

/*
 * SocketUtilities.h
 * Written by Aaryaman Sagar 
 *
 * This namespace provides basic socket functionality that aims to avoid having to
 * rewrite annoying interface socket functions over and over again
 */

#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

class SocketUtilities {
public:

    /*
     * Type Aliases
     */
    using SocketType = decltype(socket(0,0,0));
    using BufferType = std::vector<char>;

    /*
     * Standard exception class
     */
    class SocketException;

    /*
     * RAII wrapper for a socket.  The socket is automatically closed on
     * destruction of this object. 
     */
    class SocketRAII;

    /*
     * Sets the default logging output stream for this class.  Thread safe. 
     * Async unsafe
     */
    static void set_log_stream(std::ostream& log_stream_in);

    /*
     * Creates a socket on which a server may listen.  The socket is created in
     * a manner that is completely IP version agnostic and so will work with
     * IPv6 as well as IPv4
     */
    static SocketType create_server_socket(const char* port, int backlog = 10);

    /*
     * A wrapper around the recv() function that takes care of looping while
     * data has not been received, and exceptional conditions.  Use the recv()
     * version with the same intent as ::recv() and use receive() to loop and
     * receive the amount of data that was specified
     */
    static auto recv (SocketType sock_fd, void* buffer, size_t length, 
            int flags) -> decltype(::recv(0,0,0,0));
    static BufferType receive(SocketType sock_fd, size_t length, 
            unsigned flags = 0);

    /*
     * A wrapper around the send() function that takes care of looping while
     * data has not been sent and exceptional conditions
     */
    static void send(SocketType sock_fd, BufferType data_to_send);

    /*
     * A wrapper function around the accept() network call.  This method adds
     * error handling on top of the usual accept call
     */
    static SocketType accept(SocketType sock_fd, 
                             sockaddr* address = nullptr, 
                             socklen_t* address_len = nullptr);
};


/*
 * RAII Wrapper to indicate ownership of an open socket file descriptor
 */
class SocketUtilities::SocketRAII {
public:

    /*
     * Release ownership of socket, released explicitly.
     */
    void release();

    /*
     * Constructor (acquire) and destructor (release), move construction transfers
     * ownership to self and releases the ownership of socket from ther other 
     * object.
     */
    SocketRAII(int sock_fd);
    ~SocketRAII();
    SocketRAII(SocketRAII&& other);

    /*
     * Cannot copy construct from another wrapper.  This is because exclusive
     * ownership would result in scope contention (which object closes the
     * socket first) and can lead to several hard to diagnose bugs, including a
     * possibility of races amongst threads.
     *
     * Default construction is incorrect.  Something should be owned.
     *
     * Move assignment is incorrect style to initialize an object of this kind
     * in my opinion.  It implies an automatic release which the programmer may
     * have not intended to happen.
     */
    SocketRAII() = delete;
    SocketRAII(const SocketRAII&) = delete;
    SocketRAII& operator=(const SocketRAII&) = delete;
    SocketRAII& operator=(SocketRAII&&) = delete;

    /*
     * Convert to the type of the socket implicitly
     */
    operator SocketType ();

private:
    SocketType owned_socket = -1;
};

/*
 * Define this macro to enable logged output to the log stream set by the user. 
 * The default stream for log messages is stdout.  Be careful with using this
 * utility.  Output logging is not async safe and uses an internal spinlock to
 * prevent thread contention with the standard output stream for logging.
 */
#ifdef SOCKET_LOG_COMMUNICATION
    static constexpr bool log_events = true;
#else
    static constexpr bool log_events = false;
#endif

#endif
