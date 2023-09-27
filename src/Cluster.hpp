
#ifndef CLUSTER_HPP
# define CLUSTER_HPP

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
	public:
		Cluster();
		~Cluster();
		void	config(std::string configFile);
		void	print_all();
		size_t	server_size() { return _servers.size(); };
};

#endif