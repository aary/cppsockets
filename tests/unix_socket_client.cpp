#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#include <array>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include "SocketUtilities.hpp"
using namespace std;
using namespace std::literals::string_literals;
using std::array;

static const string SOCKET_PATH {"./unix_sock"};
static const string REQUEST {"Some request"};

int main() { 

    int unix_socket = SocketUtilities::create_client_unix_socket(SOCKET_PATH);

    // send data
    SocketUtilities::send_all(unix_socket, 
            REQUEST.data(), REQUEST.size());

    // now receive
    array<char, 100> buffer;
    SocketUtilities::recv(unix_socket, reinterpret_cast<void*>(buffer.data()), 
            buffer.size(), 0);

    return 0;
}
