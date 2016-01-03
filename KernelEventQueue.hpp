#ifndef __CPP_SOCKETS_KERNEL_EVENT_QUEUE_HPP__
#define __CPP_SOCKETS_KERNEL_EVENT_QUEUE_HPP__

#include "SocketUtilities.hpp"
#include <vector>

/*
 * A generic kernel event queue that is meant to be used for polling
 * purposes on a file desciptor.  The poll() and select() functions are not
 * included in this class becasue they are not made to be used in
 * multithreaded situations and do not scale well with increasing traffic. 
 * Only one kernel queue is meant to be present for one process so this will
 * be a singleton object.
 *
 * To submit file descriptors to the queue to be watched users must call
 * declare_interest(). 
 *
 * Users who want to block and get the list of file descriptors that can be
 * read must call get_readable_fds().  Similarly get_writeable_fds() will
 * return a list of file descriptors that can be written to
 *
 * This class is system dependant so it will differ from OS to OS, but in
 * most cases it will either use epoll() for linux and it will use kqueues
 * for BSDs like Darwin (and subsequently OSX)
 *
 * NOTE : These functions will fail if the socket is set to blocking.  So
 * remember to set the socket to non blocking first. 
 */

class SocketUtilities::KernelEventQueue {
public:

    using FileDescriptorType = SocketUtilities::FileDescriptorType;

    /*
     * Returns a reference to the singleton kernel event queue.  This method is
     * threadsafe.
     */
    static KernelEventQueue& get_kernel_event_queue();

    /*
     * Users may call this function to declare interest in a given file
     * descriptor.  Thread safe with respect to the internals of the queue but
     * may result in repeated additions when used concurrently without
     * synchronization
     */
    void declare_interest(FileDescriptorType);

    /*
     * This is analogous to the function call above in that this function is
     * used to "unwatch" the required file descriptor.  For example this may be
     * used when the file descriptor has been sent all required information. 
     */
    void rescind_interest(FileDescriptorType);

    /*
     * This functions is blocks to return a list of file descriptors that are
     * ready for reading or writing.  The timeout parameter will contain the
     * number of milliseconds before a timeout should occur.  And on timeout the
     * function will return an empty vector.
     */
    std::vector<FileDescriptorType> get_readable_fds();
    std::vector<FileDescriptorType> get_writeable_fds();

private:

    /*
     * The opaque pointer pimpl idiom.  Defined and declared in the
     * implementation file for this class.
     */
    class Impl;
    Impl* impl;
};

#endif
