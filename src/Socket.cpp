
#include "Socket.hpp"

Socket::Socket(std::pair<std::string, int> host_info) : _server_socket_fd(socket(AF_INET, SOCK_STREAM, 0))
{
	if ( _server_socket_fd == -1 )
	{
		std::cerr << "Socket function failed" << std::endl;
		// throw { std::exception };
	}
	bzero(&_server_address, sizeof(_server_address));

	if ( fcntl(_server_socket_fd, F_SETFL, O_NONBLOCK) < 0 )
	{
		std::cerr << "fcntl function failed" << std::endl;
	}

	int on = 1;
	if ( setsockopt(_server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 )
	{
		std::cerr << "setsockopt function failed" << std::endl;
	}

	_server_address.sin_family = AF_UNSPEC;
	_server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	_server_address.sin_port = htons(host_info.second);
}

Socket::~Socket()
{
	close(_server_socket_fd);
}