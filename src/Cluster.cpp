
#include "Cluster.hpp"

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
	// std::cout << "content : " << content << std::endl;

	std::vector<std::string> servers_config;
	servers_config = split_servers(content);

	for ( size_t i = 0 ; i < servers_config.size() ; i++ )
	{
		Server	new_server(servers_config[i]);
		_servers.push_back(new_server);
	}
}

// void	Cluster::setup()
// {
// 	int	epoll_fd = epoll_create(1);

// 	if ( epoll_fd == -1 )
// 	{
// 		std::cerr << "epoll_create failed" << std::endl;
// 		exit(EXIT_FAILURE);
// 	}

// 	set_sockets(epoll_fd);
// 	//fill socket
// 	while (1)
// 	{
// 		struct epoll_event event_list[1024];
// 		int	nb_of_events_to_handle = epoll_wait(epoll_fd, event_list, 1024, 30000);
// 		if ( nb_of_events_to_handle == -1 )
// 		{
// 			std::cerr << "kevent failed" << std::endl;
// 			exit(EXIT_FAILURE);
// 		}
// 		else if ( 0 < nb_of_events_to_handle )
// 		{
// 			for ( size_t i = 0 ; i < nb_of_events_to_handle ; i++ )
// 			{
// 				// if ( event_list[i].events == EPOLLIN || event_list[i].events == EV_WRITE )
// 				// 	std::cout << "TEST" << std::endl;
// 					//handle event
// 			}
// 		}
// 	}
// }

void	Cluster::set_sockets(int &kq)
{
	size_t	size = server_size();

	for ( size_t i = 0 ; i < size ; i++ )
	{
		std::vector<std::pair<std::string, int>> listening_port = _servers[i].get_listening_port();
		for ( size_t j = 0 ; j < listening_port.size() ; j++ )
		{
			Socket new_socket(listening_port[j]);
			_sockets.push_back(new_socket);
			struct kevent ev_set;
			EV_SET(&ev_set, new_socket.get_server_socket_fd(), EVFILT_READ, EV_ADD, 0, 0, NULL);
			if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
				std::cerr << "failed to add socket to kqueue" << std::endl;
		}
	}
}

void    Cluster::setup()
{
    int    kq = kqueue();

    if ( kq == -1 )
    {
        std::cerr << "kqueue failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    set_sockets(kq);

    while (1)
    {
		struct kevent ev_set;
        struct kevent event_list[1024];
        int    nb_of_events_to_handle = kevent(kq, NULL, 0, event_list, 1024, NULL);
        if ( nb_of_events_to_handle == -1 )
        {
            std::cerr << "kevent failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if ( 0 < nb_of_events_to_handle )
        {
            for ( size_t i = 0 ; i < nb_of_events_to_handle ; i++ )
            {
				//should check this: https://nima101.github.io/kqueue_server
				for ( size_t j = 0 ; j < get_sockets().size() ; j++ )
				{
					if ( event_list[i].ident == get_sockets()[j].get_server_socket_fd() )
					{
						socklen_t 			addr_len = sizeof(get_sockets()[j].get_server_address());
						struct sockaddr_in	addr = get_sockets()[j].get_server_address();
			
						int fd = accept(event_list[i].ident, (struct sockaddr *)&addr, &addr_len);
						if ( fd == -1 )
						{
							std::cerr << "connection refused." << std::endl;
							close(fd);
						}
						else
						{
							_clients_sockets.push_back(fd);
							EV_SET(&ev_set, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
							if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
								std::cerr << "kevent failed" << std::endl;
						}
					}
					else if (event_list[i].ident & EV_EOF)
					{
						EV_SET(&ev_set, event_list[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
						if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
							std::cerr << "kevent failed" << std::endl;
						//close and pop_back connection from _clients_sockets
					}
					else if (event_list[i].filter == EVFILT_READ)
					{
						
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