
#include "Cluster_linux.hpp"

Cluster::Cluster()
{
	_kq = epoll_create1(0);

	if ( _kq == -1 )
    {
        std::cerr << "epoll_create failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}

Cluster::~Cluster()
{
	close(_kq);
}

static void	clean_config_string(std::string &config)
{
	size_t begin;
	size_t end;

	begin = config.find('#');
	while ( begin != std::string::npos )
	{
		end = config.find('\n', begin);
		config.erase(begin, end - begin + 1);
		begin = config.find('#');
	}
	begin = config.find('\t');
	while ( begin != std::string::npos )
	{
		config.erase(begin, 1);
		begin = config.find('\t');
	}
	begin = config.find('\n');
	while ( begin != std::string::npos )
	{
		if ( config[begin + 1] == '\n' )
			config.erase(begin, 1);
		begin = config.find('\n', begin + 1);
	}
}

static std::vector<std::string>split_servers(std::string content)
{
	size_t begin;
	size_t end;
	size_t nb_of_brackets;
	std::string servor_to_push;
	std::vector<std::string> result;

	begin = content.find("server");
	while ( begin != std::string::npos )
	{
		end = content.find("{");
		nb_of_brackets = 1;
		while ( nb_of_brackets > 0 )
		{
			end++;
			if ( !content[end] )
				throw std::runtime_error("scope error");
			if ( content[end] == '{' )
				nb_of_brackets++;
			else if ( content[end] == '}' )
				nb_of_brackets--;
		}
		servor_to_push = content.substr(begin, end - begin + 1);
		content.erase(begin, end - begin + 1);
		result.push_back(servor_to_push);
		begin = content.find("server");
	}
	return result ;
}

void	Cluster::config(std::string configFile)
{
	std::ifstream		configStream(configFile.c_str());
	std::stringstream	ss;

	ss << configStream.rdbuf();

	std::string content(ss.str());
	clean_config_string(content);

	std::vector<std::string> servers_config;
	servers_config = split_servers(content);

	for ( size_t i = 0 ; i < servers_config.size() ; i++ )
	{
		Server	new_server(servers_config[i]);
		_servers.push_back(new_server);
	}
}

void	Cluster::set_sockets()
{
	size_t	size = server_size();
	struct sockaddr_in	address;
	int sock_fd;
	std::vector<std::pair<std::string, int> > listening_port;
	for ( size_t i = 0 ; i < size ; i++ )
	{
		listening_port = _servers[i].get_listening_port();
		for ( size_t j = 0 ; j < listening_port.size() ; j++ )
		{
			Socket new_socket(listening_port[j].second, &_servers[i]);
			struct epoll_event ev_set;
			ev_set.events = EPOLLIN | EPOLLOUT;
			ev_set.data.fd = new_socket.get_server_socket_fd();
			if ( epoll_ctl(_kq, EPOLL_CTL_ADD, new_socket.get_server_socket_fd(), &ev_set) )
				std::cerr << "[WEBSERV]: failed to add socket" << std::endl;
			_sockets.push_back(new_socket);
		}
	}
}

std::string readFile(std::string filename)
{
   std::stringstream buffer;
   buffer << std::ifstream( filename ).rdbuf();
   return buffer.str();
}

void	Cluster::accept_new_connection(int new_client_fd, Socket *socket)
{
	struct epoll_event ev_set;
	socklen_t addr_len = sizeof(socket->get_server_address());

	std::cerr << "[WEBSERV]: attempt to connect client from [" << new_client_fd << "]" << std::endl;
	int fd = accept(new_client_fd, (struct sockaddr *)socket->get_server_address(), &addr_len);
	if ( fd == -1 )
		std::cerr << "[WEBSERV]: connection refused." << std::endl;
	else
	{
		if ( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 )
			throw std::runtime_error("fcntl failed");

		_clients[fd].set_fd_and_server(fd, socket->get_server());
	
		ev_set.events = EPOLLIN;
		ev_set.data.fd = fd;
		if ( epoll_ctl(_kq, EPOLL_CTL_ADD, fd, &ev_set) )
			std::cerr << "[WEBSERV]: epoll_ctl failed to add client[" << new_client_fd << "]" << std::endl;
		else
			std::cerr << "[WEBSERV]: succeed to add client[" << fd << "]" << std::endl;
	}
}

Socket	*Cluster::is_a_listen_fd(int fd)
{
	for ( size_t j = 0 ; j < get_sockets().size() ; j++ )
	{
		if ( fd == get_sockets()[j].get_server_socket_fd() )
			return get_socket(j) ;
	}
	return NULL ;
}

void	Cluster::close_connection(int client_socket)
{
	if (epoll_ctl(_kq, EPOLL_CTL_DEL, client_socket, NULL) == -1)
			std::cerr << "[WEBSERV]: epoll_ctl failed to delete client [" << client_socket << "]" << std::endl;
		else
			std::cerr << "[WEBSERV]: client [" << client_socket << "] deleted successfully" << std::endl;
		close(client_socket);
		_clients.erase(client_socket);
}

int	Cluster::read_request(int client_socket)
{
	char buf[120];
	bzero(buf, 120);
	ssize_t nbytes = read(client_socket, buf, 120);
	if ( nbytes <= 0 )
	{
		close_connection(client_socket);
		return 0;
	}
	else
	{
		return _clients[client_socket].treat_received_data(buf, nbytes) ;
	}
}

void	Cluster::read_event(int client_socket)
{
	struct epoll_event ev_set;

	if ( read_request(client_socket) )
	{
		std::cerr << "[DEBUG] READ _clients[" << client_socket << "].request = " << _clients[client_socket].get_request() << "\nBody=\n[" << _clients[client_socket].get_body_request() << "]" << std::endl;
		
		_clients[client_socket].create_response();
	
		std::cerr << "\n[DEBUG] RESPONSE _clients[" << client_socket << "].response = " << _clients[client_socket].get_response() << std::endl;
		ev_set.data.fd = client_socket;
		ev_set.events = EPOLLOUT;
		if (epoll_ctl(_kq, EPOLL_CTL_MOD, client_socket, &ev_set) == -1)
			std::cerr << "[WEBSERV]: ctl mod failed" << std::endl;
	}
}

void	Cluster::write_event(int client_socket)
{
	struct epoll_event ev_set;

	if (_clients[client_socket].get_request() != "")
		std::cerr << "[DEBUG] WRITE _clients[" << client_socket << "].request = " << _clients[client_socket].get_request() << std::endl;

	// Request request();
	_clients[client_socket].handle_request();
		
	std::cerr << "[WEBSERV]: client [" << client_socket << "] Connection =[" << _clients[client_socket].get_header_request("Connection")  << "]" << std::endl;
	_clients[client_socket].get_request() = "";
	_clients[client_socket].get_body_request() = "";

	close_connection(client_socket);
}

void	Cluster::run()
{
	struct epoll_event ev_set;
    struct epoll_event event_list[1024];

	set_sockets();

    while (1)
    {
        int    nb_of_events_to_handle = epoll_wait(_kq, event_list, 1024, -1);
        if ( nb_of_events_to_handle == -1 )
        {
            std::cerr << "epoll_wait failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if ( 0 < nb_of_events_to_handle )
        {
            for ( size_t i = 0 ; i < nb_of_events_to_handle ; i++ )
            {
				Socket *socket = is_a_listen_fd(event_list[i].data.fd);
				if ( socket )
					accept_new_connection(event_list[i].data.fd, socket);
				else if ( _clients.find(event_list[i].data.fd) != _clients.end() )
				{
					if ( event_list[i].events & EPOLLIN )
						read_event(event_list[i].data.fd);
					else if ( event_list[i].events & EPOLLOUT )
						write_event(event_list[i].data.fd);
				}
            }
        }
    }
}

void	Cluster::print_all()
{
	std::cout << "====================================" << std::endl;
	std::cout << "|           Servers infos          |" << std::endl;
	std::cout << "====================================" << std::endl;

	for ( size_t i = 0 ; i < server_size() ; i++ )
	{
		std::cout << "Server      : [" << i + 1 << "]" << std::endl;
		std::cout << "root        : [" << _servers[i].get_root() << "]" << std::endl;
		std::cout << "server name : [" << _servers[i].get_server_name() << "]" << std::endl;
		std::cout << "index       : [" << _servers[i].get_index() << "]" << std::endl;
		std::cout << "redirect    : [" << _servers[i].get_redirect() << "]" << std::endl;
		std::cout << "upload_path : [" << _servers[i].get_upload_path() << "]" << std::endl;
		std::cout << "cgi_path    : [" << _servers[i].get_cgi_path() << "]" << std::endl;
	
		std::cout << "cgi ext     : [";
		for ( size_t j = 0 ; j < _servers[i].get_cgi_extension_size() ; j++ )
		{
			std::cout << _servers[i].get_cgi_extension(j);
			if ( j != _servers[i].get_cgi_extension_size() - 1 )
				std::cout << " ";
		}
		std::cout << "]" << std::endl;
	
		std::cout << "listen port : [";
		std::vector<std::pair<std::string, int> >		listening_port = _servers[i].get_listening_port();
		for ( std::vector<std::pair<std::string, int> >::iterator it = listening_port.begin() ; it != listening_port.end() ; it++ )
		{
			std::cout << it->first << "::" << it->second;
			if ( it + 1 != listening_port.end() )
				std::cout << " ";
		}
		std::cout << "]" << std::endl;

		std::cout << "methods     : [";
		if (_servers[i].get_allow_methods(GET))
			std::cout << "[GET]";
		if (_servers[i].get_allow_methods(POST))
			std::cout << "[POST]";
		if (_servers[i].get_allow_methods(DELETE))
			std::cout << "[DELETE]";
		std::cout << "]" << std::endl;
	
		std::cout << "auto index  : [";
		if (_servers[i].get_auto_index())
			std::cout << "yes";
		else
			std::cout << "no";
		std::cout << "]" << std::endl;
	
		std::cout << "body size   : [" << _servers[i].get_client_max_body_size() << "]" << std::endl;
		
		size_t	j = 1;
		std::map<std::string, Location>		locations = _servers[i].get_locations();
		for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; it++ )
		{
			std::cout << "------------------------------------" << std::endl;
			std::cout << "locations   : " << j << std::endl;
			std::cout << "root        : [" << it->second.get_root() << "]" << std::endl;
			std::cout << "server name : [" << it->second.get_server_name() << "]" << std::endl;
			std::cout << "index       : [" << it->second.get_index() << "]" << std::endl;
			std::cout << "redirect    : [" << it->second.get_redirect() << "]" << std::endl;
			std::cout << "upload_path : [" << it->second.get_upload_path() << "]" << std::endl;
			std::cout << "cgi_path    : [" << it->second.get_cgi_path() << "]" << std::endl;

			std::cout << "methods     : [";
			if (it->second.get_allow_methods(GET))
				std::cout << "[GET]";
			if (it->second.get_allow_methods(POST))
				std::cout << "[POST]";
			if (it->second.get_allow_methods(DELETE))
				std::cout << "[DELETE]";
			std::cout << "]" << std::endl;
			j++;
		}

		std::cout << "====================================" << std::endl;
	}
}

bool	Cluster::is_valid_config()
{
	for ( size_t i = 0 ; i < _servers.size() ; i++ )
	{
		if ( _servers[i].is_valid_server() == false )
			return false ;
	}
	return true ;
}