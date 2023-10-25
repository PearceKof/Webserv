
#include "Request.hpp"

Request::Request(int client_fd, client_info &client)
{
	if ( client.request == "" )
		return ;
	set_method(client.request);
	set_path(client.request, client.server->get_locations());
	set_body(client.request);
}


Request::~Request()
{
}

void	Request::set_method(std::string request)
{
	if ( request.find("GET") != std::string::npos )
		_method = "GET";
	else if ( request.find("POST") != std::string::npos )
		_method = "POST";
	else if ( request.find("DELETE") != std::string::npos )
		_method = "DELETE";
}

static	bool	is_valid_path(std::map<std::string, Location> locations, std::string path)
{
	for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; it++ )
		if ( it->first == path )
			return true ;

	return false ;
}

void	Request::set_path(std::string request, std::map<std::string, Location> locations)
{
	size_t begin = request.find("/");
	size_t end;

	if ( begin != std::string::npos )
	{
		end = request.find(' ', begin);
		_path = request.substr(begin, end - begin);
	}
	if ( !is_valid_path(locations, _path) && _method == "DELETE" )
		_path = "";
}

void	Request::set_body(std::string request)
{

}