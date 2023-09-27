
#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <sys/socket.h>

class Socket
{
private:
	int	_fd;
	struct	sockaddr_storage	_addr_info;
public:
	Socket();
	~Socket();
};


#endif