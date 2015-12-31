#ifndef __SOCKET_RAII__
#define __SOCKET_RAII__

#include "SocketUtilities.h"

/*
 * RAII Wrapper to indicate unique ownership of an open socket file descriptor,
 * this is a unique ownership class.  It does not maintain any sort of reference
 * count.  In theory it does have a binary reference count however.  Closes the
 * socket on destruction.
 */
class SocketUtilities::SocketRAII {
public:

    using SocketType = SocketUtilities::SocketType;

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
    SocketType owned_socket;
    static const SocketType null_socket; 
};

#endif
