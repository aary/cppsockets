#include "SocketRAII.h"
#include <unistd.h>

using SocketRAII = SocketUtilities::SocketRAII;
using SocketType = SocketRAII::SocketType;

/******************************************************************************
 *                       The RAII wrapper for sockets                         *
 ******************************************************************************/
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
 *                      /The RAII wrapper for sockets                         *
 ******************************************************************************/
