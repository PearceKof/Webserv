
#ifndef SOCKET_HPP
# define SOCKET_HPP

# include "Webserv.hpp"
# include <netinet/in.h>

class Server;

class Socket
{
	private:
		int					_server_socket_fd;
		struct sockaddr_in	_server_address;
		Server				*_server;

		Socket();

	public:
		Socket(int port, Server *server);
		~Socket();

		int					get_server_socket_fd() {return _server_socket_fd ; }
		struct sockaddr_in	*get_server_address() { return &_server_address ; }
		Server				*get_server() { return _server ; }
};


#endif