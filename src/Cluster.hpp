
#ifndef CLUSTER_HPP
# define CLUSTER_HPP

# include <sys/event.h>
# include "Server.hpp"
# include "Socket.hpp"
# include "Request.hpp"
# include <sstream>
# include <fstream>
# include <string>
# include <map>
class Server;
class Socket;

struct client_info
{
	struct	kevent		events;
	std::string			request;
};

class Cluster
{
	private:
		std::vector<Server>			_servers;
		std::vector<Socket>			_sockets;
		std::map<int, client_info>	_clients;
		std::vector<int>			_clients_sockets;

		void	set_sockets(int &kq);
		void	run(int &kq);
	public:
		Cluster();
		~Cluster();
	
		void	config(std::string configFile);
		void	setup_and_run();
		void	print_all();
	
		size_t				server_size() { return _servers.size(); };
		std::vector<Socket> get_sockets() { return _sockets ; };
		std::vector<int>	&get_clients_sockets() { return _clients_sockets ; };
		int					get_client_socket(int index) { return _clients_sockets[index] ; };
};

#endif