
#ifndef CLUSTER_HPP
# define CLUSTER_HPP
# include <sys/event.h>
# include "Server.hpp"
# include "Socket.hpp"
# include <sstream>
# include <fstream>
# include <string>
# include <map>
class Server;
class Socket;

class Cluster
{
	private:
		std::vector<Server>	_servers;
		std::vector<Socket>	_sockets;
		std::vector<int>	_clients_sockets;

		void	set_sockets(int &kq);
	public:
		Cluster();
		~Cluster();
		void	config(std::string configFile);
		void	setup();

		void	print_all();
		size_t	server_size() { return _servers.size(); };
		std::vector<Socket> get_sockets() { return _sockets }
		std::vector<int>	get_clients_sockets() { return _clients_sockets ; };
		int					get_client_socket(int index) { return _clients_sockets[index] ; };
};

#endif