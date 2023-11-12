
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

class Server;
class Socket;

struct client_info
{
	struct	kevent		events;
	std::string			request;
	Server				*server;
	int					socket;
};

class Cluster
{
	private:
		std::vector<Server>			_servers;
		std::vector<Socket>			_sockets;
		std::map<int, client_info>	_clients;
		std::vector<int>			_clients_sockets;
		int							_kq;

		void	set_sockets();
		void	accept_new_connection(int new_client_fd, Socket *socket);
		Socket	*is_a_listen_fd(int event_fd);
		void	delete_client(struct kevent ev_list);

	public:
		Cluster();
		~Cluster();
	
		void	config(std::string configFile);
		void	print_all();
		bool	is_valid_config();
		void	setup_and_run();
		void	run();
	
		size_t				server_size() { return _servers.size(); };
		Socket				*get_socket(int index) { return &_sockets[index] ; };
		std::vector<Socket> get_sockets() { return _sockets ; };
		std::vector<int>	&get_clients_sockets() { return _clients_sockets ; };
		int					get_client_socket(int index) { return _clients_sockets[index] ; };
};

#endif