
#include "Location.hpp"

Location::Location() {}

Location::Location(std::string location_config) : _auto_index(false), _upload(false)
{
	set_attributs(location_config);
	set_error_pages(location_config);
	set_allow_methods(location_config);
}

Location::~Location() {}

void	Location::set_attributs(std::string& location_config)
{
	_server_name = trim_config("server_name", location_config);
	_root = trim_config("root", location_config);
	_index = trim_config("index", location_config);
	_redirect = trim_config("redirect", location_config);
	_upload_path = trim_config("upload_path", location_config);
	_cgi_path = trim_config("cgi_path", location_config);
	_client_max_body_size = trim_config("client_max_body_size", location_config);

	if ( !(trim_config("autoindex", location_config).compare("on")) )
		_auto_index = true;
	if ( !(trim_config("autoindex", location_config).compare("on")) )
		_upload = true;
}

void	Location::set_error_pages(std::string location_config)
{
	std::string	error_path;
	std::string	tmp;
	size_t		begin;
	size_t		end;
	int			error_index;

	begin = location_config.find("error_page");
	while ( begin != std::string::npos )
	{
		end = location_config.find('\n');
		tmp = location_config.substr(begin, end - begin);
		location_config.erase(begin, end - begin + 1);
		begin = 11;
		end = tmp.find(' ', begin);
		error_path = tmp.substr(begin, end - begin);
		if ( error_path.find_first_not_of("0123456789") == std::string::npos )
		{
			begin = tmp.find('/', end);
			end = tmp.find('\n', begin);
			std::istringstream(error_path) >> error_index;
			_error_pages[error_index] = tmp.substr(begin, end - begin);
		}
		begin = location_config.find("error_page");
	}
}

void	Location::set_allow_methods(std::string location_config)
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