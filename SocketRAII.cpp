#include "SocketRAII.hpp"
#include <unistd.h>
#include <limits>

using SocketUtilities::SocketType;

// This is a null socket, this should be used to denote a non existing socket
// as it maps to the highest possible value of an int
static const SocketType null_socket = std::numeric_limits<SocketType>::max();

void SocketUtilities::SocketRAII::release() {
    if (this->owned_socket != null_socket) {
        close(this->owned_socket); 
        this->owned_socket = null_socket; 
    }
}

SocketUtilities::SocketRAII::SocketRAII(int sock_fd) : owned_socket{sock_fd} {}
SocketUtilities::SocketRAII::~SocketRAII() { 
    this->release(); 
}
SocketUtilities::SocketRAII::SocketRAII(SocketRAII&& other) {
    this->owned_socket = other.owned_socket;
    other.owned_socket = null_socket;
}

SocketUtilities::SocketRAII::operator SocketType() { 
    return this->owned_socket; 
}
