
#ifndef CLUSTER_LINUX_HPP
# define CLUSTER_LINUX_HPP

# include "Webserv.hpp"
# include <sys/epoll.h>
# include <sstream>
# include <fstream>
# include <vector>
# include <map>

class Server;
class Socket;

struct client_info
{
	struct	epoll_event	events;
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
		std::vector<int>	_clients_sockets;

		void	set_sockets(int &kq);
		void	accept_new_connection(int new_client_fd, int epoll_fd, Socket *socket);
		Socket	*is_a_listen_fd(int event_fd);
	public:
		Cluster();
		~Cluster();
		void	config(std::string configFile);
		bool	is_valid_config();
		void	setup_and_run();
		void	run(int &kq);

		void	print_all();
		size_t	server_size() { return _servers.size(); };
		Socket				*get_socket(int index) { return &_sockets[index] ; };
		std::vector<Socket> get_sockets() { return _sockets ; };
		std::vector<int>	&get_clients_sockets() { return _clients_sockets ; };
		int					get_client_socket(int index) { return _clients_sockets[index] ; };
};

#endif