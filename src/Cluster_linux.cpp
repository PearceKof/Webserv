
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

		_clients[fd].socket = fd;
		_clients[fd].server = socket->get_server();
		_clients[fd].body_is_unfinished = false;
		_clients[fd].left_to_read = 0;
		_clients[fd].request_is_chunked = false;
	
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

void    Cluster::setup_and_run()
{

    set_sockets();

	run();
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


int Cluster::read_body(client_info &client, ssize_t nbytes, char *buf)
{
	if ( client.left_to_read <= 0 && !client.request_is_chunked )
		return 0;
	if ( client.server->get_client_max_body_size() != 0 && client.server->get_client_max_body_size() < (client.body_request.size() + nbytes) )
	{
		client.max_body_size_reached = true;
		client.left_to_read = 0;
		return 0;
	}

	for ( ssize_t i = 0 ; i < nbytes ; i++ )
		client.body_request.push_back(buf[i]);
	
	if ( client.left_to_read )
	{
		client.left_to_read -= nbytes;
		// std::cerr << "\n\n[DEBUG]: left_to_read " << client.left_to_read << "\n\n" << std::endl;
		return client.left_to_read > 0 ;
	}
	else if ( client.request_is_chunked )
	{
		bool	end_of_file_reached = ( client.body_request.find("\r\n0\r\n\r\n") != std::string::npos );
		return end_of_file_reached ;
	}
	return 1 ;

}

void Cluster::parse_request(client_info &client)
{
	std::stringstream ss(client.request);
	std::string	key_header, value_header;

	ss >> client.method >> client.path >> client.version;

	while ( getline(ss, key_header, ':') && getline(ss, value_header, '\r') )
	{
		value_header.erase(0, value_header.find_first_not_of(" \r\n\t"));
		value_header.erase(value_header.find_last_not_of(" \r\n\t") + 1, value_header.length());
		key_header.erase(0, key_header.find_first_not_of(" \r\n\t"));
		client.header_request[key_header] = value_header;
	}

	if ( client.header_request.find("Content-Length") != client.header_request.end() )
	{
		std::stringstream ssbs(client.header_request["Content-Length"]);
		ssbs >> client.body_size;
	}
	else
		client.body_size = 0;
	
	if ( client.header_request.find("Transfer-Encoding") != client.header_request.end() )
	{
		client.request_is_chunked = ( client.header_request["Transfer-Encoding"].find("chunked") != std::string::npos );
	}
	client.left_to_read = client.body_size;
}

int	Cluster::read_request(client_info &client)
{
	char buf[120];
	bzero(buf, 120);
	ssize_t nbytes = read(client.socket, buf, 120);
	if ( nbytes <= 0 )
	{
		close_connection(client.socket);
		return 0;
	}

	// std::cerr << "[DEBUG]: client[" << client.socket << "] readed \n[" << buf << "]\n\n" << std::endl;

	if ( client.body_is_unfinished )
		return !read_body(client, nbytes, buf);

	std::string	tmp = (client.request + (std::string)buf);
	size_t pos_end_header = tmp.find("\r\n\r\n");


	if ( pos_end_header == std::string::npos )
	{
		for ( ssize_t i = 0 ; i < nbytes ; i++ )
			client.request.push_back(buf[i]);

		return 0 ;
	}
	else
	{
		client.request += tmp.substr(client.request.size(), pos_end_header - client.request.size());

		client.body_request = tmp.substr(pos_end_header + 4, tmp.size() - (pos_end_header + 4));
		
		parse_request(client);
		client.left_to_read -= client.body_request.size();

		client.body_is_unfinished = client.left_to_read > 0;
		return !client.body_is_unfinished;
	}
}

void	Cluster::put_back_chunked(client_info &client)
{
	std::string	line;
	std::stringstream ss(client.body_request);

	while ( getline(ss, line) )
	{
		int chunk_size = std::atoi(line.c_str());
		if ( chunk_size == 0 )
			break ;
		std::string	chunk;
		chunk.resize(chunk_size);
		ss.read(&chunk[0], chunk_size);
		std::cerr << "[DEBUG]: chunk= " << chunk << std::endl;
	}

}

std::string	Cluster::create_response(client_info &client)
{
	std::string response;

	if ( client.request_is_chunked )
		put_back_chunked(client);
	if ( client.method.empty() || client.path.empty() || client.version.empty() )
		return error(400);
	if ( client.max_body_size_reached )
	{
		return error(413);
	}
	else if ( client)
	return response ;
}

void	Cluster::read_event(int client_socket)
{
	struct epoll_event ev_set;

	if ( read_request(_clients[client_socket]) )
	{
		std::cerr << "[DEBUG] READ _clients[" << client_socket << "].request = " << _clients[client_socket].request << "\nBody=\n[" << _clients[client_socket].body_request << "]" << std::endl;
		
		_clients[client_socket].response = create_response(_clients[client_socket]);
		std::cerr << "\n[DEBUG] RESPONSE _clients[" << client_socket << "].response = " << _clients[client_socket].response << std::endl;
		ev_set.data.fd = client_socket;
		ev_set.events = EPOLLOUT;
		if (epoll_ctl(_kq, EPOLL_CTL_MOD, client_socket, &ev_set) == -1)
			perror("epoll_ctl");
	}
}

void	Cluster::write_event(int client_socket)
{
	struct epoll_event ev_set;

	if (_clients[client_socket].request != "")
		std::cerr << "[DEBUG] WRITE _clients[" << client_socket << "].request = " << _clients[client_socket].request << std::endl;

	Request request(_clients[client_socket]);
	request.handle_request(_clients[client_socket]);
		
	std::cerr << "[WEBSERV]: client [" << client_socket << "] Connection =[" << _clients[client_socket].header_request["Connection"]  << "]" << std::endl;
	_clients[client_socket].request = "";
	_clients[client_socket].body_request = "";

		close_connection(client_socket);
}

void	Cluster::run()
{
	struct epoll_event ev_set;
    struct epoll_event event_list[1024];
	struct sockaddr_in *addr;

    while (1)
    {
        int    nb_of_events_to_handle = epoll_wait(_kq, event_list, 1024, -1);
		std::cerr << "[DEBUG]: epoll wait have " << nb_of_events_to_handle << " event to handle" << std::endl;
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