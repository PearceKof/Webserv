
#include "Server.hpp"

Server::Server(std::string server_config) : _auto_index(false), _upload(false), _client_max_body_size(1000000)
{
	set_locations(server_config);
	set_attributs(server_config);
	set_allow_methods(server_config);
	set_listen(server_config);
	set_cgi_extension(server_config);
}

Server::~Server()
{
}

void	Server::set_locations(std::string& server_config)
{
	size_t	begin;
	size_t	end;
	std::string	path;
	std::string	tmp;

	begin  = server_config.find("location");
	while ( begin != std::string::npos )
	{
		end = server_config.find('}');
		tmp = server_config.substr(begin, end - begin + 1);
		server_config.erase(begin, end - begin + 1);
		begin = tmp.find('/');
		end = tmp.find('{');
		path = tmp.substr(begin, end - begin - 1);
		begin = tmp.find('{') + 1;
		end = tmp.find('}');
		Location location(tmp.substr(begin, end - begin));
		_locations[path] = location;
		begin = server_config.find("location");
	}
}

static int	set_client_max_body_size(std::string size_str)
{
	size_t	multi = 1;
	int		value;

	if ( size_str.find_first_not_of("0123456789") != std::string::npos &&
		size_str.find_first_not_of("0123456789") != size_str.size() - 1 )
		return 0 ;
	else if ( size_str[size_str.size() - 1] != 'M' && size_str[size_str.size() - 1] != 'K' )
		return 0 ;
	else if ( size_str[size_str.size() - 1] == 'K' )
	{
		size_str.erase(size_str.size() - 1, std::string::npos);
		multi = 1000;
	}
	else if ( size_str[size_str.size() - 1] == 'M' )
	{
		size_str.erase(size_str.size() - 1, std::string::npos);
		multi = 1000000;
	}
	if (size_str.size() > 4)
		return 0 ;
	std::istringstream(size_str) >> value;
	return value * multi ;
}

void	Server::set_attributs(std::string& server_config)
{

	_server_name = trim_config("server_name", server_config);
	_root = trim_config("root", server_config);
	_index = trim_config("index", server_config);
	_redirect = trim_config("redirect", server_config);
	_upload_path = trim_config("upload_path", server_config);
	_cgi_path = trim_config("cgi_path", server_config);

	_client_max_body_size =  set_client_max_body_size(trim_config("client_max_body_size", server_config));
	if ( _client_max_body_size == 0 )
		std::cerr << "Wrong body size." << std::endl;
	
	if ( !(trim_config("autoindex", server_config).compare("on")) )
		_auto_index = true;
	if ( !(trim_config("autoindex", server_config).compare("on")) )
		_upload = true;
	
}

void	Server::set_error_pages(std::string server_config)
{
	size_t	begin;
	size_t	end;
	std::string	error_path;
	std::string	tmp;
	int	error_index;

	begin = server_config.find("error_page");
	while ( begin != std::string::npos + 11 )
	{
		end = server_config.find('\n');
		tmp = server_config.substr(begin, end - begin);
		server_config.erase(begin, end - begin + 1);
		begin += 11;
		end = tmp.find(' ', begin);
		error_path = tmp.substr(begin, end - begin);
		if (error_path.find_first_not_of("0123456789") == std::string::npos) {
			begin = tmp.find('/', end);
			end = tmp.find('\n', begin);
			std::istringstream(error_path) >> error_index;
			_error_pages[error_index] = tmp.substr(begin, end - begin);
		}
		begin = begin = server_config.find("error_page");
	}
}

void	Server::set_allow_methods(std::string location_config)
{
	size_t		begin;
	size_t		end;
	std::string	methods;

	begin = location_config.find("allow_methods") + 14;
	if ( begin != std::string::npos + 14 )
	{
		end = location_config.find("\n");
		methods = location_config.substr(begin, end - begin);
	}
	for ( int i = 0 ; i < 3 ; i++ )
		_allow_methods[i] = false;
	if ( methods.find("GET") != std::string::npos )
		_allow_methods[GET] = true;
	if ( methods.find("POST") != std::string::npos )
		_allow_methods[POST] = true;
	if ( methods.find("DELETE") != std::string::npos )
		_allow_methods[DELETE] = true;
}

static bool	is_valid_host(std::string host)
{
	int	nb_of_point = 0;

	if ( host == "localhost" )
		return true ;
	for ( size_t i = 0 ; host[i] && nb_of_point < 4 ; i++ )
		if (host[i] == '.')
			nb_of_point++;

	if ( nb_of_point != 3 )
		return false ;
	
	std::string	host_checker;
	size_t		begin = 0;
	size_t		end = 0;
	int			value_checker;

	for ( size_t i = 0 ; i < 4 ; i++ )
	{
		end = host.find('.', begin);
		if ( end == std::string::npos && i < 3 )
			return false ;
		host_checker = host.substr(begin, end - begin);
		if ( host_checker.find_first_not_of("0123456789") != std::string::npos )
			return false ;
		std::istringstream(host_checker) >> value_checker;
		if ( value_checker < 0 && 255 < value_checker )
			return false ;
		begin = end + 1;
	}
	return true ;
}

void	Server::set_listen(std::string location_config)
{
	size_t		begin;
	size_t		end;
	std::string	tmp;
	std::string	host;
	std::string	str_port;
	int			port;

	begin = location_config.find("listen") + 7;
	end = location_config.find('\n', begin);
	tmp = location_config.substr(begin, end - begin);
	while ( tmp.find(':') != std::string::npos )
	{
		end = tmp.find(':');
		host = tmp.substr(0, end);
		begin = end + 1;
		end = tmp.find(' ', begin);
		if ( end == std::string::npos )
			end = tmp.find('\n', begin);
		str_port = tmp.substr(begin, end - begin);
		if ( is_valid_host(host) == true
			&& str_port.find_first_not_of("0123456789") == std::string::npos
			&& str_port.size() <= 4)
		{
			if ( host == "127.0.0.1" )
				host = "localhost";
			std::istringstream(str_port) >> port;
			std::pair<std::string, int>	pair_host_port(host, port);
			_listening_port.push_back(pair_host_port);
		}
		while ( end < tmp.size() - 1 && tmp[end] == ' ')
			end++;
		tmp.erase(0, end);
	}
}

void	Server::set_cgi_extension(std::string server_config)
{
	size_t		begin;
	size_t		end;
	std::string	tmp;

	begin = server_config.find("cgi_extension") + 14;
	if ( begin == std::string::npos + 14 )
		return ;
	end = server_config.find('\n', begin);
	tmp = server_config.substr(begin, end - begin);
	begin = 0;
	end = tmp.find(' ', begin);
	while ( end != std::string::npos ) //à checker
	{
		_cgi_extension.push_back(tmp.substr(begin, end - begin));
		begin = end + 1;
		end = tmp.find(' ', begin);
	}
	_cgi_extension.push_back(tmp.substr(begin, end - begin));
}
