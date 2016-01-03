#include <iostream>
#include <thread>
#include <fstream>
#include <array>
#include <stdexcept>
#include <sstream>
#include "SocketUtilities.hpp"
using namespace std;

const string response = 
"HTTP/1.1 200 OK\n\n"
"Hello, World!";

int main(int argc, char** argv) {
    
    // Error check command line arguments
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port_number>" << endl;
        exit(1);
    }

    // create socket on which the server will listen
    using SocketUtilities::SocketRAII;
    SocketRAII sockfd = SocketUtilities::create_server_socket(argv[1]);

    // Print serving prompt
    cout << " * Serving on port " << argv[1] << " (Press CTRL+C to quit)" << endl;

    while (true) {  // main accept() loop

        // block and accept connection
        auto new_fd = SocketUtilities::accept(sockfd);

        // receive data in a seperate thread in a non blocking manner
        std::thread ([](decltype(new_fd) new_fd) {

            SocketRAII auto_close(new_fd);

            array<char, 1024> buffer;
            SocketUtilities::recv(new_fd, buffer.data(), buffer.size());

            // send data
            std::vector<char> data_to_send(response.begin(), response.end());
            SocketUtilities::send_all(new_fd, data_to_send);

        }, new_fd).detach(); 
    }

    return 0;
}

