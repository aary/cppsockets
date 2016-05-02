#include <vector>
#include <array>
#include <string>
#include <iostream>
#include "SocketUtilities.hpp"
using namespace std;

/* A standard http get request from curl */
static const string request {"GET / HTTP/1.1\n"
"Host: localhost:8000\n"
"User-Agent: curl/7.43.0\n"
"Accept: */*\n\n"
};

int main(int argc, char *argv[]) {

    if (argc != 3) {
        cerr << "Usage " << argv[0] << " <remote_host> <port>" << endl;
        exit(1);
    }

    using SocketUtilities::SocketRAII;
    SocketRAII client_socket = 
        SocketUtilities::create_client_socket(argv[1], argv[2]);
    
    // send request
    SocketUtilities::send_all(client_socket, 
            reinterpret_cast<const void*>(request.data()), request.size());

    // receive response, this will print the received string to stdout if
    // compiled with the macro SOCKET_LOG_COMMUNICATION defined
    array<char, 1024> buffer;
    SocketUtilities::recv(client_socket, buffer.data(), buffer.size());

    return 0;
}
