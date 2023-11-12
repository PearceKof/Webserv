
#include "Request.hpp"

Request::Request(client_info &client)
{
	if ( client.request == "" )
		return ;

	_request_a_file = false;

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
	std::string extension = "default";

	if ( begin != std::string::npos )
	{
		end = request.find(' ', begin);
		_path = request.substr(begin, end - begin);
	}
	if ( !is_directory(_path) && !is_valid_path(locations, _path) )
		_path = "null";
	if ( _mime.is_a_file(request) )
	{
		_request_a_file = true;
		begin = request.find('/');
		end = request.find(' ', begin);
		_path = request.substr(begin, end - begin);
		begin = request.find('.');
		end = request.find(' ', begin);
		extension = request.substr(begin, end - begin);
	}

	_content_type = _mime.get_content_type(extension);
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
			map_value.erase(0, map_value.find_first_not_of(" \r\n\t"));
			map_value.erase(map_value.find_last_not_of(" \r\n\t") + 1);

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

void	redirection(int client_sock, std::string redirection)
{
	std::string	response("HTTP/1.1 301 Moved Permanently\r\n");
	response += "Location: ";
	response += redirection;
	response += "\r\n";
	response += "Content-Length: 0\r\n\r\n";
	send(client_sock, response.c_str(), response.size(), 0);
}

void	Request::send_auto_index(client_info client)
{
	_path = "." + _path;
	DIR *dir = opendir(_path.c_str());
	if ( dir == NULL )
	{
		std::cerr << "error 403" << std::endl;
		return ;
	}

	std::string response = "http/1.1 200 OK\r\n";
	response += "Date: " + daytime() + "\r\n";
	response += "Server: " + client.server->get_server_name() + "\r\n";
	response += "Content-Type: text/html\r\n";
	response += "Connection: Close\r\n";
	std::string body = "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><table>";
	body += "<tr><td><a href=\"../\">../</a></td><td>-</td></tr>";

	struct dirent *dirent;

	while ( (dirent = readdir(dir)) != NULL )
	{
		std::string name = dirent->d_name;
		std::string path = _path + "/" + name;

		std::string size;
		if ( dirent->d_type == DT_REG)
		{
			struct stat st;
			if (stat(path.c_str(), &st) == 0)
			{
				std::stringstream ss;
				ss << st.st_size;
				size = ss.str() + " bytes";
			}
		}
		else
			size = "-";
		
		if ( dirent->d_type != DT_DIR )
			body += "<tr><td><a href=\"" + _path + "/" + name + "\">" + name + "</a></td><td>" + size + "</td></tr>";
	}
	body += "</table></body></html>";
	closedir(dir);
	response += "Content-lenght: " + std::to_string(body.size()) + "\r\n\r\n";
	response += body;
	size_t nbyte = response.size();
	while ( nbyte > 0 )
		nbyte -= send(client.socket, response.c_str(), response.size(), 0);
}

void error(client_info client, int status_code)
{}

void	Request::get_method(client_info client)
{
	Location location = client.server->get_locations()[_path];
	// std::cerr << "is " << _path << " a directory: " << is_directory(_path) << std::endl;
	if ( is_directory(_path) )
	{
		if ( location.get_auto_index() == true || client.server->get_auto_index() == true )
			send_auto_index(client);
		std::cerr << "[DEBUG]: autoindex end" << std::endl;
		return ;
	}
	if ( _path == "null" )
		send_response(client, "404 not found", "text/html", client.server->get_root() + client.server->get_error_page(404));
	else if ( _request_a_file == true )
		send_response(client, "200 OK", _content_type, _path);
	else if ( _method == "GET" )
	{

		if ( location.get_redirect() != "" )
		{
			redirection(client.socket, location.get_redirect());
			return ;
		}

		std::string	content = location.get_root();
		if ( content == "" )
			content = client.server->get_root();
		
		if ( location.get_allow_methods(GET) == false && client.server->get_allow_methods(GET) == false )
		{
			if ( location.get_error_page(405) != "" )
				content += location.get_error_page(405);
			else
				content += client.server->get_error_page(405);
			send_response(client, "405 Method Not Allowed", "text/html", content);
		}
		else
		{
			if ( location.get_index() != "" )
				content += location.get_index();
			else
				content += client.server->get_index();

			send_response(client, "200 OK", "text/html", content);
		} 

	}
}

void loadFile(const std::string &fileName, std::stringstream &stream) {
	std::ifstream input_file(fileName.c_str());
	stream << input_file.rdbuf();
	input_file.close();
}

void	Request::send_image(client_info client, std::string image, std::string response)
{
	std::cerr << "send_image" << std::endl;
	FILE	*img_file = fopen(image.c_str(), "rb");
	if ( img_file == NULL )
	{
		_request_a_file = false;
		send_response(client, "404 not found", _content_type, "");
		return ;
	}
	fseek(img_file, 0L, SEEK_END);
	size_t	size = ftell(img_file);
	response += "Content-Lenght: " + std::to_string(size) + "\r\n\r\n";
	fclose(img_file);

	std::ifstream stream;
	stream.open(image, std::ifstream::binary);
	if ( stream.is_open() )
	{
		char *buffer = new char[size];
		bzero(buffer, size);
		stream.read(buffer, size);

		std::cerr << "send_image debug a " << std::to_string(size) << " response: " <<  response << std::endl;

		for (size_t i = 0; i < size ; i++)
			response.push_back(buffer[i]);

		ssize_t ret = response.size();
		while (ret > 0)
			ret -= send(client.socket, response.c_str(), response.size(), 0);
		stream.close();
		delete[] buffer;
		std::cerr << "send_image debug b " << std::to_string(size) << " response: " <<  response << std::endl;
		buffer = nullptr;
	}
}

void	Request::send_response(client_info client, std::string status_code, std::string content_type, std::string file)
{
	std::string response = "http/1.1 " + status_code + "\r\n";
	response += "Date: " + daytime() + "\r\n";
	response += "Connection: keep-alive\r\n";
	response += "Server: " + client.server->get_server_name() + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";

	std::cerr << "DEBUG" << std::endl;
	std::string content;
	if ( _request_a_file == true )
	{
		send_image(client, ASSETS_DIR + file, response);
		return ;
	}
	else if ( file != "" )
	{
		std::ifstream content_stream(file.c_str());
		std::stringstream stream;
		stream << content_stream.rdbuf();
		content = stream.str();
	}
	else
		content = "<h1>ERROR 404 - Page not found</h1>";
	response += "Content-Length: " + std::to_string(content.size()) + "\r\n\n";
	response += content + "\r\n";

	std::cerr << "[DEBUG] response sended:\n" << response << std::endl;
	size_t nbyte = response.size();
	while ( nbyte > 0 )
		nbyte -= send(client.socket, response.c_str(), response.size(), 0);


}