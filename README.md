# cppsockets

This project aims to provide an object based interface to the traditional
socket I/O system calls for both TCP and Unix sockets in such a way that is
manageable and clean.

## Sample server using this library

```C++
#include <iostream>
#include <thread>
#include <fstream>
#include <array>
#include <stdexcept>
#include <sstream>
#include "SocketUtilities.hpp"
using namespace std;

static const string response {
"HTTP/1.1 200 OK\n\n"
"Hello, World!"
};

int main(int argc, char** argv) {

    // Error check command line arguments
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <port_number>" << endl;
        return 1;
    }

    // create socket on which the server will listen
    using SocketUtilities::SocketRAII;
    SocketRAII sockfd {SocketUtilities::create_server_socket(argv[1])};

    // Print serving prompt
    cout << " * Serving on port " << argv[1] << " (Press CTRL+C to quit)" << endl;

    while (true) {  // main accept() loop

        // block and accept connection
        auto new_fd = SocketUtilities::accept(sockfd);

        // receive data in a non blocking manner
        std::thread ([](auto new_fd) {

            SocketRAII auto_close {new_fd};

            array<char, 1024> buffer;
            SocketUtilities::recv(new_fd, buffer.data(), buffer.size());

            // send data
            SocketUtilities::send_all(new_fd,
                reinterpret_cast<const void*>(response.data()),
                response.size());

        }, new_fd).detach();
    }

    return 0;
}
```

To compile and run it yourself type in `make sampleserver && ./sampleserver
8000`.  Use curl as a client to this `curl --request GET
"http://localhost:8000"`

## Installation

To install this library for use with your project, either first add it as a
submodule or download/clone this repository and follow the steps below
```shell
git submodule add https://github.com/aary/cppsockets.git submodules/cppsockets
git submodule update --init --recursive
```

And then install it like so it
```shell
cd submodules/cppsockets
make install
```

This will produce an archive called `libcppsockets.a` that you should simply
move to the folder with the rest of your code.  Linking with this archive will
link with the objects for this library.  This will also produce symlinks to
all the header files that you may need.

**This is not a header only library, so you will have to link with
`libcppsockets.a`.**

The header file `SocketUtilities.hpp` should be included wherever you use the
functionality provided in this library.  Consider making a link to this
library with the `-I` flag to `g++` or `clang++` when compiling.  

An example build process to use this library would be
```shell
cd submodules/cppsockets
make install
mv libcppsockets.a ../../myproject
cd ../myproject
g++ -std=c++14 -I ../submodules my_network_program.cpp libcppsockets.a
```
With `my_network_program.cpp` including the headers for this library like so
```C++
#include "cppsockets/SocketUtilities.hpp"
```

## Directory organization
The directories for this project have been organized as follows

- `./include` Contains symlinks to all the header files that are public for
  this library `make install` will create symlinks in the root directory for
  this project for easy `#include`ing
- `./src` contains all the private source code for this library.  Symlinks to
  the headers in the include/ directory are present along with symlinks to
  some tests
- `./tests` contains test cases

## Requirements
This library has no external requirements other than the C++ standard library.
Just follow the steps in installation section to install this library for use
with your projects.

## Contributing
Please fork this repository and contribute back using pull requests. Features
can be requested using issues. All code, comments, and critiques are greatly
appreciated.
