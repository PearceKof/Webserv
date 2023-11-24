
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
# include <vector>
# include <stack>

class Server;
class Socket;
class Request;

class Cluster
{
	private:
		std::vector<Server>			_servers;
		std::vector<Socket>			_sockets;
		std::map<int, Request>		_clients;
		int							_kq;

		void	set_sockets();
		void	accept_new_connection(int new_client_fd, Socket *socket);
		void	close_connection(int client_socket);
		Socket	*is_a_listen_fd(int event_fd);
		void	read_event(int client_socket);
		int		read_request(int client_socket);
		void	write_event(int client_socket);

	public:
		Cluster();
		~Cluster();
	
		void	config(std::string configFile);
		bool	is_valid_config();
		void	run();

		void	print_all();
		size_t	server_size() { return _servers.size(); };
		Socket				*get_socket(int index) { return &_sockets[index] ; };
		std::vector<Socket> get_sockets() { return _sockets ; };
		// std::vector<int>	&get_clients_sockets() { return _clients_sockets ; };
		// int					get_client_socket(int index) { return _clients_sockets[index] ; };
		Server				*get_server(int index) { return &_servers[index] ; };
		std::vector<Server> get_servers() { return _servers ; };
};

#endif