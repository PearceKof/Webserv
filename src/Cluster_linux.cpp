#include "Cluster_linux.hpp"

Cluster::Cluster()
{
}

Cluster::~Cluster()
{
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

void	Cluster::set_sockets(int &fd)
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
			if ( epoll_ctl(fd, EPOLL_CTL_ADD, new_socket.get_server_socket_fd(), &ev_set) )
				std::cerr << "failed to add socket" << std::endl;
			_sockets.push_back(new_socket);
			
			std::cerr << "DEBUG i: " << i << " j: " << j << " fd: " << _sockets.back().get_server_socket_fd() << std::endl;
		}
	}
}

std::string readFile(std::string filename)
{
   std::stringstream buffer;
   buffer << std::ifstream( filename ).rdbuf();
   return buffer.str();
}

void	Cluster::accept_new_connection(int new_client_fd, int epoll_fd, Socket *socket)
{
	struct epoll_event ev_set;
	socklen_t addr_len = sizeof(socket->get_server_address());

	std::cerr << "client attempt to connect " << new_client_fd << std::endl;
	int fd = accept(new_client_fd, (struct sockaddr *)socket->get_server_address(), &addr_len);
	if ( fd == -1 )
	{
		std::cerr << "connection refused." << std::endl;
		close(fd);
	}
	else
	{
		if ( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 )
		{
			throw std::runtime_error("fcntl function failed");
		}
		_clients_sockets.push_back(fd);
		ev_set.events = EPOLLIN | EPOLLOUT;
		ev_set.data.fd = fd;
		_clients[fd].events = ev_set;
		_clients[fd].server = socket->get_server();
		if ( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev_set) )
			std::cerr << "failed to add client" << std::endl;
		else
			std::cerr << "succeed to add client : " << fd << std::endl;
	}
}

Socket	*Cluster::is_a_listen_fd(int event_fd)
{
	for ( size_t j = 0 ; j < get_sockets().size() ; j++ )
	{
		if (event_fd == get_sockets()[j].get_server_socket_fd())
			return get_socket(j) ;
	}
	return NULL ;
}

void    Cluster::setup()
{
    int    epoll_fd = epoll_create1(0);

    if ( epoll_fd == -1 )
    {
        std::cerr << "epoll_create failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    set_sockets(epoll_fd);

	struct epoll_event ev_set;
    struct epoll_event event_list[1024];
	struct sockaddr_in *addr;
    while (1)
    {
        int    nb_of_events_to_handle = epoll_wait(epoll_fd, event_list, 1024, 30000);
        if ( nb_of_events_to_handle == -1 )
        {
            std::cerr << "kevent failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if ( 0 < nb_of_events_to_handle )
        {
            for ( size_t i = 0 ; i < nb_of_events_to_handle ; i++ )
            {
				Socket *socket = is_a_listen_fd(event_list[i].data.fd);
				if ( socket )
					accept_new_connection(event_list[i].data.fd, epoll_fd, socket);
				else if ( std::find(get_clients_sockets().begin(), get_clients_sockets().end(), event_list[i].data.fd) != get_clients_sockets().end() )
				{
					if ( event_list[i].events & EPOLLIN )
					{
						_clients[event_list[i].data.fd].request = read_request(event_list[i].data.fd);
						if (_clients[event_list[i].data.fd].request != "")
							std::cerr << "[DEBUG] READ _clients[" <<event_list[i].data.fd<< "].request = " << _clients[event_list[i].data.fd].request << std::endl;
					}
					else if ( event_list[i].events & EPOLLOUT )
					{
						Request(event_list[i].data.fd, _clients[event_list[i].data.fd]);
						// if (_clients[event_list[i].data.fd].request != "")
						// 	std::cerr << "[DEBUG] WRITE _clients[" << event_list[i].data.fd << "].request = " << _clients[event_list[i].data.fd].request << std::endl;
					}
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