#include "Cluster.hpp"
#include <sys/types.h>
#include <sys/socket.h>

extern int run;

Cluster::Cluster()
{
	_kq = kqueue();
	if ( _kq == -1 )
    {
        std::cerr << "[ERROR]: kqueue failed" << std::endl;
        exit(EXIT_FAILURE);
    }
}
#include <stack>
Cluster::~Cluster()
{
	std::cout << "[WEBSERV]: Cluster destructer called" << std::endl;

	if ( not _clients.empty() )
	{
		std::stack<int> stack;
		for ( std::map<int, Request>::iterator it = _clients.begin() ; it != _clients.end() ; ++it )
			stack.push(it->first);
		while ( not stack.empty() )
		{
			close_connection(stack.top());
			stack.pop();
		}
	}

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

	for ( size_t i = 0 ; i < size ; i++ )
	{
		std::vector<std::pair<std::string, int> > listening_port = _servers[i].get_listening_port();
		for ( size_t j = 0 ; j < listening_port.size() ; j++ )
		{
			Socket new_socket(listening_port[j].second, &_servers[i]);
			_sockets.push_back(new_socket);
			struct kevent ev_set;
			EV_SET(&ev_set, new_socket.get_server_socket_fd(), EVFILT_READ, EV_ADD, 0, 0, NULL);
			if ( kevent(_kq, &ev_set, 1, NULL, 0, NULL) == -1 )
				std::cerr << "[WEBSERV]: failed to add socket to kqueue" << std::endl;
			else
				std::cerr << "[WEBSERV]: succeed to add socket to kqueue" << std::endl;
			
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
	struct kevent ev_set;
	socklen_t addr_len = sizeof(socket->get_server_address());

	std::cerr << "[WEBSERV]: attempt to connect client from [" << new_client_fd << "]" << std::endl;
	int fd = accept(new_client_fd, (struct sockaddr *)socket->get_server_address(), &addr_len);
	if ( fd == -1 )
		throw std::runtime_error("connection refused");
	else
	{
		if ( fcntl(fd, F_SETFL, O_NONBLOCK) < 0 )
			throw std::runtime_error("fcntl function failed");


		EV_SET(&ev_set, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
		if ( kevent(_kq, &ev_set, 1, NULL, 0, NULL) == -1 )
			std::cerr << "[WEBSERV]: kevent failed to add client [" << fd << "]" << std::endl;
		EV_SET(&ev_set, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
		if ( kevent(_kq, &ev_set, 1, NULL, 0, NULL) == -1 )
			std::cerr << "[WEBSERV]: kevent failed to add client [" << fd << "]" << std::endl;

		_clients[fd].set_fd_and_server(fd, socket->get_server(), *this);
		std::cerr << "[WEBSERV]: succeed to add client [" << fd << "]" << std::endl;
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

void	Cluster::close_connection(int client_socket)
{
	struct kevent ev_set[2];
	EV_SET(ev_set, client_socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	EV_SET(ev_set + 1, client_socket, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	if ( kevent(_kq, ev_set, 2, NULL, 0, NULL) < 0 )
		std::cerr << "[WEBSERV]: failed to delete client [" << client_socket << "]" << std::endl;
	else
	{
		close(client_socket);
		_clients.erase(client_socket);
		std::cerr << "[WEBSERV]: client [" << client_socket << "] deleted successfully" << std::endl;
	}
}

int	Cluster::read_request(int client_socket)
{
	char buf[120];
	bzero(buf, 120);
	ssize_t nbytes = read(client_socket, buf, 120);
	if ( nbytes <= 0 )
	{
		if ( nbytes == -1 )
			std::cerr << "[ERROR]: client [" << client_socket << "] failed to read request" << std::endl;
		close_connection(client_socket);
		return 0;
	}
	else
		return _clients[client_socket].treat_received_data(buf, nbytes) ;
}

void	Cluster::read_event(int client_socket)
{
	struct kevent ev_set;

	std::cerr << "[DEBUG]: read called for client [" << client_socket << "]" << std::endl;
	if ( read_request(client_socket) )
	{
		_clients[client_socket].create_response();
		_clients[client_socket].set_ready(true);
	}
}

void	Cluster::write_event(int client_socket)
{
	// std::cerr << "[DEBUG]: write for client [" << client_socket << "] response = " << _clients[client_socket].get_response()  << std::endl;
	if ( _clients[client_socket].is_ready_to_send() )
	{
		std::cerr << "[DEBUG]: client [" << client_socket << "] is ready to send" << std::endl;
		if ( _clients[client_socket].send_response() )
		{
			std::cerr << "TEST connection == [" << _clients[client_socket].get_header_request("Connection") << "]"<< std::endl;
			if (_clients[client_socket].get_header_request("Connection") != "keep-alive")
				close_connection(client_socket);

			_clients[client_socket].get_response() = "";
			_clients[client_socket].get_request() = "";
			_clients[client_socket].get_body_request() = "";
			_clients[client_socket].set_ready(false);
		}
	}
}

void	Cluster::run()
{
	struct kevent ev_list[1024];
	extern int run;
	set_sockets();

	while ( run )
    {
        int    num_events = kevent(_kq, NULL, 0, ev_list, 1024, NULL);
		if ( not run )
			std::cerr << "[WEBSERV]: signal to shutdown received" << std::endl;
        else if ( num_events == -1 )
        {
            std::cerr << "[ERROR]: kevent failed" << std::endl;
			run = 0;
        }
        else if ( 0 < num_events )
        {
            for ( size_t i = 0 ; i < num_events ; i++ )
            {

				Socket *socket = is_a_listen_fd(ev_list[i].ident);
				if ( socket )
					accept_new_connection(ev_list[i].ident, socket);
				else if ( _clients.find(ev_list[i].ident) != _clients.end() )
				{
					if (ev_list[i].filter == EVFILT_READ)
						read_event(ev_list[i].ident);
					else if (ev_list[i].filter == EVFILT_WRITE)
						write_event(ev_list[i].ident);
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
		// std::cout << "index       : [" << _servers[i].get_index() << "]" << std::endl;
	
		std::cout << "listen port : [";
		std::vector<std::pair<std::string, int> >		listening_port = _servers[i].get_listening_port();
		for ( std::vector<std::pair<std::string, int> >::iterator it = listening_port.begin() ; it != listening_port.end() ; it++ )
		{
			std::cout << it->first << "::" << it->second;
			if ( it + 1 != listening_port.end() )
				std::cout << " ";
		}
		std::cout << "]" << std::endl;
	
		std::cout << "body size   : [" << _servers[i].get_client_max_body_size() << "]" << std::endl;
		
		size_t	j = 1;
		std::map<std::string, Location>		locations = _servers[i].get_locations();
		for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; it++ )
		{
			std::cout << "------------------------------------" << std::endl;
			std::cout << "locations   : " << j << std::endl;
			std::cout << "root        : [" << it->second.get_root() << "]" << std::endl;
			std::cout << "index       : [" << it->second.get_index() << "]" << std::endl;
			std::cout << "redirect    : [" << it->second.get_redirect() << "]" << std::endl;
			std::cout << "upload_path : [" << it->second.get_upload_path() << "]" << std::endl;

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
	if ( _servers.size() == 0 )
		return false ;
	return true ;
}