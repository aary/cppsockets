The header file is self explanatory and well documented.

A sample server is included in tcp_socket_server.cpp.  To compile type in 

`g++ -std=c++11 tcp_socket_server.cpp SocketUtilities.cpp`  
Use curl as a client to this  
`curl --request GET "http://localhost:8000"`  
