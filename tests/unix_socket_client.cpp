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

static const string SOCKET_PATH {"./unix_sock"};
static const string REQUEST {"Some request"};
int create_client_unix_socket(const string& socket_path);

int main() { 

    int unix_socket;
    try {
        unix_socket = create_client_unix_socket(SOCKET_PATH);
    } catch (std::exception& exc) {
        cerr << exc.what();
        return 1;
    }
    cout << "Created unix socket on file descriptor " << unix_socket << endl;

    // send data
    if (::send(unix_socket, REQUEST.data(), REQUEST.size(), 0) == -1) {
        cerr << "Error in send() : " << strerror(errno) << endl;
        return 1;
    }

    // now receive
    array<char, 100> buffer;
    int bytes_received = ::recv(unix_socket, 
            reinterpret_cast<void*>(buffer.data()), 
            buffer.size(), 0);
    if (bytes_received == -1) {
        cerr << "Error in recv() : " << strerror(errno) << endl;
        return 1;
    }
    cout << "Received data : [";
    string received_data(buffer.data(), buffer.data() + bytes_received);
    cout << received_data << "]" << endl;

    return 0;
}

int create_client_unix_socket(const string& socket_path) {

    // STEP 1 : socket()
    int unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        throw std::runtime_error {"Error in socket() call : "s + 
            string(strerror(errno))};
    }
    
    // STEP 2 : connect()
    sockaddr_un remote_address;
    remote_address.sun_family = AF_UNIX;
    std::copy(socket_path.begin(), socket_path.end(), 
            remote_address.sun_path);

    if (::connect(unix_socket, 
                reinterpret_cast<sockaddr*>(&remote_address), 
                sizeof(remote_address)) == -1) {
        throw std::runtime_error {"Error in connect() call : "s + 
            string(strerror(errno))};
    }

    return unix_socket;
}
