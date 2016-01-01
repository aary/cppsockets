#ifndef __SOCKET_UTILITIES__HPP__
#define __SOCKET_UTILITIES__HPP__

/*
 * SocketUtilities.hpp 
 * Written by Aaryaman Sagar
 *
 * This class provides basic socket functionality that aims to avoid having to
 * rewrite annoying Berkeley TCP stream socket functions over and over again.
 *
 * The class has been made to resemble the old berkeley sockets interface, i.e.
 * the class has been designed as a namespace with limited behavior, such as the
 * having of a default logging stream and its own internal mutexes and spinlocks
 * to prevent interleaving.
 *
 * Note: The functions have logging disabled by default.  To enable compile with
 * the -DSOCKET_LOG_COMMUNICATION flag to g++.  When logging is enabled the
 * functions are no longer async safe.  You have been warned.
 *
 * The header is arranged as follows. 
 *
 *      1. Types and aliases come first
 *
 *      2. Basic socket network functions, such as accept(), recv() and send()
 *
 *      3. Other advanced techniques such as polling in a non blocking manner
 *         with timeouts
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

    using SocketType = decltype(socket(0,0,0));     // socket file descriptors
    using FileDescriptorType = SocketType;          // generic file descriptors
    using BufferType = std::vector<char>;           // a generic buffer type

    /*
     * RAII Wrapper to indicate unique ownership of an open socket file descriptor,
     * this is a unique ownership class.  It does not maintain any sort of reference
     * count.  In theory it does have a binary reference count however.  Closes the
     * socket on destruction.  Incomplete type has been defined elsewhere in
     * SocketRAII.h and implemented in its own implmentation file.
     */
    class SocketRAII;

    /*
     * Standard exception class, all exceptions thrown are of this type.
     * Inherits from std::exception.  This is an incomplete type that is defined
     * by the implementation.
     */
    class SocketException;

    /*
     * Creates a socket on which a server may listen.  The socket is created in
     * a manner that is completely IP version agnostic and so will work with
     * IPv6 as well as IPv4
     */
    static SocketType create_server_socket(const char* port, int backlog = 10);

    /*
     * Create a socket though which a client connects to a server on the
     * network. This like the server equivalent of the same function is also IP
     * version agnostic.  getaddrinfo() is called and then the return ed lined
     * listis iterated over to find the address values that can be connect()ed
     * to
     */
    static SocketType create_client_socket(const char* address, 
                                           const char* port);

    /*
     * A wrapper around the recv() function that takes care of looping while
     * data has not been received, and exceptional conditions.  
     *
     * Exceptional conditions are not tolerated while calling recv() for stream
     * sockets.  In the case of a non blocking socket, if there is a potential
     * blocking situation then the user did not call poll() correctly and did
     * not perform the poll() operation correctly, either using the normal
     * system call or the wrapper provided in this module.
     *
     * Use the recv() version with the same intent as ::recv() 
     * 
     * Use recv_all to receive the amount of data that was specified.  This
     * blocks while all the data that has been requested has not been received.
     */
    static auto recv (SocketType sock_fd, void* buffer, size_t length, 
            int flags = 0) -> decltype(::recv(0,0,0,0));
    static BufferType recv_all(SocketType sock_fd, size_t length, 
            unsigned flags = 0);  // utilize move semantics to be efficient

    /*
     * A wrapper around the send() function that takes care of looping while
     * data has not been sent and exceptional conditions.
     *
     * Keep in mind that a return value of 0 is not an exceptional condition.
     * This is in fact a feature of TCP; this may indicate that the other side
     * is not keeping up.  If the other side has ended the connection then
     * SIGPIPE would be generated if the flags do not include MSG_NOSIGNAL.
     * send_all() by default includes the MSG_NOSIGNAL flag.
     *
     * Use the send() function to get the same functionality as the ::send()
     * call.  This is also a blocking call. 
     *
     * Use the send_all() function to send all the data that is in the buffer
     * that was passed in.  This loops till all the data has been sent so this
     * function will block.
     */
    static auto send(SocketType sock_fd, void* buffer, size_t length, 
            int flags = 0) -> decltype(::send(0,0,0,0));
    static void send_all(SocketType sock_fd, const BufferType& data_to_send);

    /*
     * A wrapper function around the accept() network call.  This method adds
     * error handling on top of the usual accept call.
     *
     * This function returns the same type as does the usual accept call, the
     * way to store the returned socket in the context of this library would be
     * to assign it to a SocketRAII object which would close the socket on
     * destruction. 
     */
    static auto accept(SocketType sock_fd, sockaddr* address = nullptr, 
                       socklen_t* address_len = nullptr) 
        -> decltype(::accept(0,0,0));

    /*
     * Use these functions to poll() for data on a socket file descriptor in a 
     * non-blocking manner.  
     *
     * poll_read() will return true if there is data to be read in the socket
     * pointed to by fd.  This will throw an exception in the case of an error
     * or a timeout.
     *
     * poll_write will return true if data can be written to the socket pointed
     * to by fd.  This will throw an exception in the case of an error or a
     * timeout. 
     *
     * These are intended to be fast and portable so in most implementations of
     * this library they will use the poll() system call instead of the two
     * widely used alternatives - select() and epoll()
     *
     * NOTE : These functions will fail if the socket is set to blocking.  So
     * remember to set the socket to non blocking first. 
     */
    static bool poll_read(SocketType sock_fd);    // can data be read
    static bool poll_write(SocketType sock_fd);   // can data be written
    static void make_non_blocking(SocketType sock_fd);

    /*
     * Sets the default logging output stream for this class.  Thread safe. 
     * Async unsafe.  Cannot use this function safely from within signal
     * handlers.  As a result of every function making calls to log events, this
     * class should be used with care in an signal handling environment. 
     */
    static void set_log_stream(std::ostream& log_stream_in);

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

#endif // __SOCKET_UTILITIES__

/*
 * Include the SocketRAII header for convenience
 */
#include "SocketRAII.hpp"

