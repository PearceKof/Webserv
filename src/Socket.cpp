
#include "Socket.hpp"

Socket::Socket(int port, Server *server) : _server_socket_fd(socket(AF_INET, SOCK_STREAM, 0)), _server(server) 
{
	if ( _server_socket_fd == -1 )
	{
		throw std::runtime_error("Socket function failed");
	}
	bzero(&_server_address, sizeof(_server_address));

	if ( fcntl(_server_socket_fd, F_SETFL, O_NONBLOCK) < 0 )
	{
		throw std::runtime_error("fcntl function failed");
	}

	int on = 1;
	if ( setsockopt(_server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 )
	{
		throw std::runtime_error("setsockopt function failed");
	}

	_server_address.sin_family = AF_UNSPEC;
	_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_server_address.sin_port = htons(port);

	if ( bind(_server_socket_fd, (struct sockaddr *)&_server_address, sizeof(_server_address)) < 0 )
		perror("bind:");

	if ( listen(_server_socket_fd, 1000) < 0 )
	{
		throw std::runtime_error("listen function failed");
	}

}

Socket::~Socket() {}