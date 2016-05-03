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
using namespace std;
using namespace std::literals::string_literals;
using std::array;

int create_server_unix_socket(const string& socket_path);
static const string SOCKET_FILE {"./unix_sock"};

int main() {
    
    int unix_socket;
    try {
        unix_socket = create_server_unix_socket(SOCKET_FILE);
    } catch (std::exception& exc) {
        cerr << exc.what();
        return 1;
    }
    cout << "Created socket on file descriptor " << unix_socket << endl;

    // main accept loop
    while (true) { 

        // accept a new connection
        char answer;
        cout << "Do you want the server to continue (y/n)? ";
        cin >> answer;
        if (answer == 'n') { break; }

        cout << "Accepting connection ..." << endl;
        auto sock = ::accept(unix_socket, nullptr, nullptr);
        if (sock == -1) {
            cerr << "Error occured in accept() call : " << std::strerror(errno)
                << endl;
            return 1;
        }
        
        // accept data and echo a response back
        array<char, 100> buffer;
        int bytes_received = ::recv(sock, 
                reinterpret_cast<void*>(buffer.data()), buffer.size(), 0);
        if (bytes_received == -1) {
            cerr << "Error in recv() call : " << strerror(errno) << endl;
            return 1;
        }

        // print the data received
        cout << "Received " << bytes_received << " bytes : [";
        string data_received(buffer.data(), buffer.data() + bytes_received);
        cout << data_received << "]" << endl;

        // send response back
        const string response {"Received your message!"};
        ::send(sock, reinterpret_cast<const void*>(response.data()), 
                response.size(), 0);

        // close socket
        ::close(sock);
    }

    // close the server socket
    ::close(unix_socket);
    ::unlink(SOCKET_FILE.c_str());

    return 0;
}

int create_server_unix_socket(const string& socket_path) {
    
    // STEP 1 : socket()
    int unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        throw std::runtime_error {"Error occured in socket() call : "s + 
            string(std::strerror(errno))};
    }

    // STEP 2 : bind(), setup the address structures for bind()
    sockaddr_un local_address;
    local_address.sun_family = AF_UNIX;
    std::copy(socket_path.begin(), socket_path.end(), 
            local_address.sun_path);

    // unlink from before
    ::unlink(socket_path.c_str());

    // STEP 2 : bind()
    if (::bind(unix_socket, reinterpret_cast<sockaddr*>(&local_address), 
                sizeof(local_address)) == -1) {
        throw std::runtime_error {"Error occured in bind() call : "s + 
            string(strerror(errno))};
    }

    // STEP 3 : listen()
    if (listen(unix_socket, 5) == -1) {
        throw std::runtime_error {"Error occured in listen() call : "s + 
            string(strerror(errno))};
    }

    return unix_socket;
}
