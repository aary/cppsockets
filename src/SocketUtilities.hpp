#pragma once

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
#include <sys/fcntl.h>
#include <atomic>

/*
 * Main namespace.  Every utility in this library is within this namespace.  All
 * header files and implmentation files extend this namespace.  
 */
namespace SocketUtilities {

/* socket file descriptors */
using SocketType = decltype(socket(0,0,0));
/* generic file descriptors */
using FileDescriptorType = SocketType;

/*
 * Standard exception class, all exceptions thrown are of this type.
 * Inherits from std::exception.  This is an incomplete type that is defined
 * by the implementation.
 */
class SocketException;

/*
 * Forward declarations of other classes in this library that have public
 * interfaces.  Consult respective header (.hpp) files for documentation on each
 * of these.
 */
class SocketRAII;
class KernelEventQueue;

/*
 * Creates a socket on which a server may listen.  The socket is created in
 * a manner that is completely IP version agnostic and so will work with
 * IPv6 as well as IPv4
 */
SocketType create_server_socket(const char* port, int backlog = 10);

/*
 * Create a socket though which a client connects to a server on the
 * network. This like the server equivalent of the same function is also IP
 * version agnostic.  
 *
 * getaddrinfo() is called and then the returned lined listis iterated over to
 * find the address values that can be connect()ed to
 */
SocketType create_client_socket(const char* address, const char* port);

/*
 * Creates a unix socket on which a server may listen, wait for incoming
 * connections.  The socket is created in the file specified.
 */
SocketType create_server_unix_socket(const std::string& socket_path, 
        int backlog = 10);

/*
 * Creates a client that is connected to another unix socket.  The socket path
 * should be provided to the function
 */
SocketType create_client_unix_socket(const std::string& socket_path);

/*
 * A wrapper around the recv() function that takes care of looping while data
 * has not been received, and exceptional conditions.
 *
 * Use the recv() version with the same intent as ::recv() 
 */
ssize_t recv (SocketType sock_fd, void* buffer, size_t length, int flags = 0);

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
ssize_t send(SocketType sock_fd, const void* buffer, size_t length, 
        int flags = 0);
void send_all(SocketType sock_fd, const std::vector<char>& data_to_send);
void send_all(SocketType sock_fd, const void* buffer, size_t length);

/*
 * A wrapper function around the accept() network call.  This method adds
 * error handling on top of the usual accept call.
 *
 * This function returns the same type as does the usual accept call, the
 * way to store the returned socket in the context of this library would be
 * to assign it to a SocketRAII object which would close the socket on
 * destruction. 
 */
SocketType accept(SocketType sock_fd, sockaddr* address = nullptr, 
        socklen_t* address_len = nullptr);

/*
 * Sets the socket sock_fd to be non-blocking.  Any subsequent calls to blocking
 * socket functions will return with an error and errno will be set to EGAIN or
 * EWOULDBLOCK.
 */
void make_non_blocking(SocketType sock_fd);

/*
 * Sets the default logging output stream for this class.  Thread safe.
 * Async unsafe.  Cannot use this function safely from within signal
 * handlers.  As a result of every function making calls to log events, this
 * class should be used with care in an signal handling environment. 
 */
void set_log_stream(std::ostream& log_stream_in);

/*
 * Use this to protect data races when printing to the same output stream.  This
 * sockets library uses stdout by default.  To change this call the
 * set_log_stream() function the first thing when main() starts
 */
extern std::atomic<bool> network_output_protect;

/*
 * Define this macro to enable logged output to the log stream set by the user. 
 * The default stream for log messages is stdout.  Be careful with using this
 * utility.  Output logging is not async safe and uses an internal spinlock to
 * prevent thread contention with the standard output stream for logging. 
 */
#ifdef SOCKET_LOG_COMMUNICATION
    constexpr bool log_events = true;
#else
    constexpr bool log_events = false;
#endif

}

/* Include the other headers in this library for convenience */
#include "SocketRAII.hpp"
#include "KernelEventQueue.hpp"
