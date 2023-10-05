
#ifndef SOCKET_HPP
# define SOCKET_HPP

#include "Webserv.hpp"
#include <fcntl.h>
class Socket
{
private:
	int				_server_socket_fd;
	struct sockaddr_in	_server_address;
	Socket();
public:
	Socket(std::pair<std::string, int> host_info);
	~Socket();

	struct sockaddr_in	get_server_address() { return _server_address ; }
};


#endif