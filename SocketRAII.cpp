#include "SocketRAII.hpp"
#include <unistd.h>
#include <limits>

using SocketUtilities::SocketType;
static const SocketType null_socket = std::numeric_limits<SocketType>::max();

void SocketUtilities::SocketRAII::release() {
    if (this->owned_socket != null_socket) {
        close(this->owned_socket); 
        this->owned_socket = null_socket; 
    }
}

SocketUtilities::SocketRAII::SocketRAII(int sock_fd) : owned_socket(sock_fd) {}
SocketUtilities::SocketRAII::~SocketRAII() { this->release(); }
SocketUtilities::SocketRAII::SocketRAII(SocketRAII&& other) {
    this->owned_socket = other.owned_socket;
    other.owned_socket = null_socket;
}

SocketUtilities::SocketRAII::operator SocketType () { return this->owned_socket; }
