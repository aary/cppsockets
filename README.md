# cppsockets

This project aims to provide an object based interface to the traditional
socket I/O system calls in such a way that is manageable and clean.

### Sample server using this library

```C++
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
"Hello, World!\n";

int main(int argc, char** argv) {

    // Error check command line arguments
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port_number>" << endl;
        exit(1);
    }

    // create socket on which the server will listen
    using SocketRAII = SocketUtilities::SocketRAII;
    SocketRAII sockfd = SocketUtilities::create_server_socket(argv[1]);

    // Print serving prompt
    cout << " * Serving on port " << argv[1] << " (Press CTRL+C to quit)" 
        << endl;

    // main accept() loop
    while (true) {  

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
```

To compile and run it yourself type in `make sampleserver && ./sampleserver
8000`.  Use curl as a client to this `curl --request GET
"http://localhost:8000"`

### Installation

To install this library in your system type the following make command
```
make install
```

This will produce an archive called cppsockets.a that you should simply move
to the folder with the rest of your code.  The header file
`SocketUtilities.hpp` should be included wherever you use the functionality
provided in this library.  Consider making a link to this header with the `-I`
flag to `g++` or `clang++` when compiling.


### License 

Copyright (c) 2015 Aaryaman Sagar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
