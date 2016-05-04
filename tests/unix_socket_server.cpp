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
#include <fstream>
#include "SocketUtilities.hpp"
using namespace std;
using namespace std::literals::string_literals;
using std::array;

static const string SOCKET_FILE {"./unix_sock"};

int main() {
    
    // create a server unix socket
    int unix_socket = SocketUtilities::create_server_unix_socket(SOCKET_FILE);

    // main accept loop
    while (true) { 

        // accept a new connection
        char answer;
        cout << "Do you want the server to continue (y/n)? "; 
        cin >> answer;
        if (answer == 'n') { break; }

        cout << "Accepting connection ..." << endl;
        auto sock_fd = SocketUtilities::accept(unix_socket);
        
        // accept data and echo a response back
        array<char, 100> buffer;
        SocketUtilities::recv(sock_fd, 
                reinterpret_cast<void*>(buffer.data()), buffer.size());

        // send response back
        static const string response {"Received your message!"};
        SocketUtilities::send_all(sock_fd, 
                reinterpret_cast<const void*>(response.data()), 
                response.size());

        // close socket
        ::close(sock_fd);
    }

    // close the server socket
    ::close(unix_socket);
    ::unlink(SOCKET_FILE.c_str());

    return 0;
}
