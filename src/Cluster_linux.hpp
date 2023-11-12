
#ifndef CLUSTER_LINUX_HPP
# define CLUSTER_LINUX_HPP

# include "Webserv.hpp"
# include <sys/epoll.h>
# include <sstream>
# include <fstream>
# include <vector>
# include <map> 
# include "Request.hpp"

class Server;
class Socket;
class Request;

struct client_info
{
	std::string			request;
	std::string			response;
	std::map<std::string , std::string>	header_request;
	std::string			body_request;
	Server				*server;
	int					socket;

	std::string			method;
	std::string			version;
	std::string			path;
	int					body_size;
	int					left_to_read;
	bool				request_is_chunked;
	bool				body_is_unfinished;
	bool				max_body_size_reached;
};

class Cluster
{
	private:
		std::vector<Server>			_servers;
		std::vector<Socket>			_sockets;
		std::map<int, client_info>	_clients;
		int							_kq;

		void	set_sockets();
		void	accept_new_connection(int new_client_fd, Socket *socket);
		void	close_connection(int client_socket);
		Socket	*is_a_listen_fd(int event_fd);
		void	read_event(int client_socket);
		int		read_request(client_info &client);
		int 	read_body(client_info &client, ssize_t nbytes, char *buf);
		void 	parse_request(client_info &client);
		std::string	create_response(client_info &client);
		void	put_back_chunked(client_info &client);

		void	write_event(int client_socket);

	public:
		Cluster();
		~Cluster();
	
		void	config(std::string configFile);
		bool	is_valid_config();
		void	setup_and_run();
		void	run();

		void	print_all();
		size_t	server_size() { return _servers.size(); };
		Socket				*get_socket(int index) { return &_sockets[index] ; };
		std::vector<Socket> get_sockets() { return _sockets ; };
		// std::vector<int>	&get_clients_sockets() { return _clients_sockets ; };
		// int					get_client_socket(int index) { return _clients_sockets[index] ; };
};

#endif