
#include "Request.hpp"

Request::Request(int client_fd, client_info &client)
{
	if ( client.request == "" )
		return ;
	set_method(client.request);
	set_path(client.request, client.server->get_locations());
	set_header_and_body(client.request);
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

void	Request::set_header_and_body(std::string request)
{
	std::stringstream	ss(request);
	std::string	line;

	while ( std::getline(ss, line) )
	{
		if ( line == "\r")
			break ;

		size_t colon_pos = line.find(':');
		if ( colon_pos != std::string::npos )
		{
			std::string map_key = line.substr(0, colon_pos);
			std::string	map_value = line.substr(colon_pos + 1);
			map_value.erase(0, map_value.find_first_not_of(" \t"));
			map_value.erase(map_value.find_last_not_of(" \t") + 1);

			_header[map_key] = map_value; 
		}
	}
	size_t	boundary_pos = request.find("boundary=") + 9;
	_header["boundary"] = request.substr(boundary_pos, request.find("\r\n", boundary_pos));

	_body = request.substr(request.find("\r\n", boundary_pos) + 2, std::string::npos);
}

void	Request::handle_request(client_info client)
{
	if (_method == "GET")
		get_method(client);
	// else if (_method == "DELETE")
	// 	delete_method(client);
	// else if (_method == "POST")
	// 	post_method(client);

}

void	Request::get_method(client_info client)
{
	if (_path == "")
	{
		// erreur 404
		// send_response(client->server.get_root());
	}
	else if (_method == "GET")
	{
		Location location = client->server.get_locations()[_path];

		if ( location.get_redirect() != "" )
		{
			redirection();
			return ;
		}

		std::string	content = location.get_root();
		if ( content == "" )
			content = client->server.get_root();
		
		if ( location.get_method(GET) == false && client->server.get_method(GET) )
		{
			//error 403
		}


	}
}

void	Request::send_response(client_info client, std::string status_code, std::string content_type)
{
	std::string response = "http/1.1" + status_code + "\r\n";
	response += "Date: " + get_time_of_day() + "\r\n";
	response += "Server: " + client->server.get_server_name() + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";

	std::string content;
}