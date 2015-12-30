#include <iostream>
#include <array>
#include <stdexcept>
#include <sstream>
#include "SocketUtilities.h"
using namespace std;

string response = 
"HTTP/1.1 200 OK\n\n"
"Hello, World!\n";

int main() {
    using SocketRAII = SocketUtilities::SocketRAII;

    // create socket on which the server will listen
    SocketRAII sockfd = SocketUtilities::create_server_socket("8000");

    cout << "Serving on port " << "8000" << endl;

    array<char, 1024> buffer;
    while (true) {  // main accept() loop

        // block and accept connection
        SocketRAII new_fd = SocketUtilities::accept(sockfd, 
                                                    (sockaddr *) nullptr, 
                                                    (socklen_t*) nullptr); 

        // receive data
        SocketUtilities::recv(new_fd, buffer.data(), buffer.size(), 0);

        // send data
        std::vector<char> data_to_send(response.begin(), response.end());
        SocketUtilities::send(new_fd, data_to_send);
    }

    return 0;
}

