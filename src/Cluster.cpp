#include "Cluster.hpp"
#include <sys/types.h>
#include <sys/socket.h>

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

void	Cluster::set_sockets(int &kq)
{
	size_t	size = server_size();

	for ( size_t i = 0 ; i < size ; i++ )
	{
		std::vector<std::pair<std::string, int> > listening_port = _servers[i].get_listening_port();
		for ( size_t j = 0 ; j < listening_port.size() ; j++ )
		{
			Socket new_socket(listening_port[j].second, &_servers[i]);
			_sockets.push_back(new_socket);
			struct kevent ev_set;
			EV_SET(&ev_set, new_socket.get_server_socket_fd(), EVFILT_READ, EV_ADD, 0, 0, NULL);
			if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
				std::cerr << "failed to add socket to kqueue" << std::endl;
		}
	}
}

std::string readFile(std::string filename)
{
   std::stringstream buffer;
   buffer << std::ifstream( filename ).rdbuf();
   return buffer.str();
}

void    Cluster::setup_and_run()
{
    int    kq = kqueue();

    if ( kq == -1 )
    {
        std::cerr << "kqueue failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    set_sockets(kq);

	run(kq);
}

void	Cluster::accept_new_connection(int new_client_fd, int kq, Socket *socket)
{
	struct kevent ev_set;
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
		ev_set.ident = fd;
		EV_SET(&ev_set, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		_clients[fd].socket = fd;
		_clients[fd].server = socket->get_server();
		_clients[fd].events = ev_set;
		if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
			std::cerr << "kevent failed to add" << std::endl;
		else
			std::cerr << "new connection accepted" << std::endl;
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

void	Cluster::delete_client(struct kevent ev_list, int kq)
{
	struct kevent ev_set;
	std::cerr << "DELETE client: " << ev_list.ident << std::endl;
	ev_set = _clients[ev_list.ident].events;
	EV_SET(&ev_set, ev_list.ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	if ( kevent(kq, &ev_set, 1, NULL, 0, NULL) == -1 )
		std::cerr << "kevent failed to delete" << std::endl;
	_clients.erase(ev_list.ident);
	close(ev_list.ident);
}

void	Cluster::run(int &kq)
{
	while (1)
    {
		struct kevent ev_set;
        struct kevent ev_list[1024];
        int    num_events = kevent(kq, NULL, 0, ev_list, 1024, NULL);
        if ( num_events == -1 )
        {
            std::cerr << "kevent failed" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if ( 0 < num_events )
        {
            for ( size_t i = 0 ; i < num_events ; i++ )
            {
				std::cerr << "[DEBUG] event nb: " << i << " event: " << ev_list[i].filter << std::endl;
				Socket *socket = is_a_listen_fd(ev_list[i].ident);
				if ( socket )
					accept_new_connection(ev_list[i].ident, kq, socket);
				else if ( _clients.find(ev_list[i].ident) != _clients.end() )
				{
					int filter = ev_list[i].filter;
					if (filter == EVFILT_READ)
					{
						_clients[ev_list[i].ident].request = read_request(ev_list[i].ident);

						if (_clients[ev_list[i].ident].request != "")
							std::cerr << "[DEBUG] READ _clients[" <<ev_list[i].ident<< "].request = " << _clients[ev_list[i].ident].request << std::endl;

						if (_clients[ev_list[i].ident].request == "")
							delete_client(ev_list[i], kq);
						else
						{
							ev_set = _clients[ev_list[i].ident].events;
							EV_SET(&ev_set, ev_list[i].ident, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, 0);
							if ( kevent(kq, &ev_set, 1, 0, 0, 0 ) )
								std::cerr << "kevent failed" << std::endl;
						}
					}
					else if (filter == EVFILT_WRITE)
					{
						if (_clients[ev_list[i].ident].request != "")
							std::cerr << "[DEBUG] WRITE _clients[" << ev_list[i].ident << "].request = " << _clients[ev_list[i].ident].request << std::endl;

						Request request(ev_list[i].ident, _clients[ev_list[i].ident]);

						request.handle_request(_clients[ev_list[i].ident]);
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