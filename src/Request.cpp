
#include "Request.hpp"

Request::Request()
{
	_body_is_unfinished = false;
	_left_to_read = 0;
	_request_is_chunked = false;
	_request_a_file = false;

	_socket = 0;
	_body_size = 0;
	_left_to_read = 0;
	_left_to_send = 0;

	_request_a_file = false;
	_request_is_chunked = false;
	_body_is_unfinished = false;
	_max_body_size_reached = false;
}

void	Request::set_fd_and_server(int fd, Server *server)
{
	_socket = fd;
	_server = server;
}

static	bool	is_valid_path(std::map<std::string, Location> locations, std::string path)
{
	for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; it++ )
		if ( it->first == path )
			return true ;

	return false ;
}

void	Request::set_path(std::map<std::string, Location> locations)
{
	size_t begin = _request.find("/");
	size_t end;
	std::string extension = "default";

	if ( begin != std::string::npos )
	{
		end = _request.find(' ', begin);
		_path = _request.substr(begin, end - begin);
	}
	if ( !is_directory(_path) && !is_valid_path(locations, _path) )
		_path = "";
	if ( _mime.is_a_file(_request) )
	{
		_request_a_file = true;
		begin = _request.find('/');
		end = _request.find(' ', begin);
		_path = _request.substr(begin, end - begin);
		begin = _request.find('.');
		end = _request.find(' ', begin);
		extension = _request.substr(begin, end - begin);
	}
	_content_type = _mime.get_content_type(extension);
}

Request::~Request()
{
}

int Request::read_body( ssize_t nbytes, char *buf)
{
	if ( _left_to_read <= 0 && !_request_is_chunked )
		return 0;
	if ( _server->get_client_max_body_size() != 0 && _server->get_client_max_body_size() < (_body_request.size() + nbytes) )
	{
		_max_body_size_reached = true;
		_left_to_read = 0;
		return 0;
	}

	for ( ssize_t i = 0 ; i < nbytes ; i++ )
		_body_request.push_back(buf[i]);
	
	if ( _left_to_read )
	{
		_left_to_read -= nbytes;
		// std::cerr << "\n\n[DEBUG]: left_to_read " << _left_to_read << "\n\n" << std::endl;
		return _left_to_read > 0 ;
	}
	else if ( _request_is_chunked )
	{
		bool	end_of_file_reached = ( _body_request.find("\r\n0\r\n\r\n") != std::string::npos );
		return end_of_file_reached ;
	}
	return 1 ;

}

void Request::parse_request()
{
	std::stringstream ss(_request);
	std::string	key_header, value_header;

	ss >> _method >> _path >> _version;

	while ( getline(ss, key_header, ':') && getline(ss, value_header, '\r') )
	{
		value_header.erase(0, value_header.find_first_not_of(" \r\n\t"));
		value_header.erase(value_header.find_last_not_of(" \r\n\t") + 1, value_header.length());
		key_header.erase(0, key_header.find_first_not_of(" \r\n\t"));
		_header_request[key_header] = value_header;
	}

	if ( _header_request.find("Content-Length") != _header_request.end() )
	{
		std::stringstream ssbs(_header_request["Content-Length"]);
		ssbs >> _body_size;
	}
	else
		_body_size = 0;
	
	if ( _header_request.find("Transfer-Encoding") != _header_request.end() )
	{
		_request_is_chunked = ( _header_request["Transfer-Encoding"].find("chunked") != std::string::npos );
	}

	size_t	boundary_pos = _request.find("boundary=") + 9;
	if ( _request.find("boundary=") != std::string::npos + 9 )
	{
		_header_request["boundary"] = _request.substr(boundary_pos, _request.find("\r\n", boundary_pos));
	}
	_left_to_read = _body_size;
}

int	Request::treat_received_data(char *buf, ssize_t nbytes)
{
	// std::cerr << "[DEBUG]: client[" << _socket << "] readed \n[" << buf << "]\n\n" << std::endl;

	if ( _body_is_unfinished )
		return !read_body(nbytes, buf);

	std::string	tmp = (_request + (std::string)buf);
	size_t pos_end_header = tmp.find("\r\n\r\n");

	if ( pos_end_header == std::string::npos )
	{
		for ( ssize_t i = 0 ; i < nbytes ; i++ )
			_request.push_back(buf[i]);

		return 0 ;
	}
	else
	{
		_request += tmp.substr(_request.size(), pos_end_header - _request.size());

		_body_request = tmp.substr(pos_end_header + 4, tmp.size() - (pos_end_header + 4));
		
		parse_request();
		_left_to_read -= _body_request.size();

		_body_is_unfinished = _left_to_read > 0;
		return !_body_is_unfinished;
	}
}

void	Request::put_back_chunked()
{
	std::string	line;
	std::stringstream ss(_body_request);

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

std::string	Request::create_response()
{
	std::string response;

	set_path(_server->get_locations());

	if ( _request_is_chunked )
		put_back_chunked();
	if ( _method.empty() || _path.empty() || _version.empty() )
	{
		std::cerr << "[DEBUG]: error 400 here" << std::endl;
		// return error(400);
	}
	if ( _max_body_size_reached )
	{
		std::cerr << "[DEBUG]: error 413 here" << std::endl;
		// return error(413);
	}

	_left_to_send = response.size();
	return response ;
}

void	Request::handle_request()
{
	if (_method == "GET")
		handle_GET();
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

void	Request::send_auto_index()
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
	response += "Server: " + _server->get_server_name() + "\r\n";
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
		nbyte -= send(_socket, response.c_str(), response.size(), 0);
}

void error(int status_code)
{}

void	Request::handle_GET()
{
	Location location = _server->get_locations()[_path];
	// std::cerr << "is " << _path << " a directory: " << is_directory(_path) << std::endl;
	if ( is_directory(_path) )
	{
		if ( location.get_auto_index() == true || _server->get_auto_index() == true )
			send_auto_index();
		std::cerr << "[DEBUG]: autoindex end" << std::endl;
		return ;
	}
	if ( _path == "" )
	{
		//error(404);
		send_response("404 not found", "text/html", _server->get_root() + _server->get_error_page(404));
	}
	else if ( _request_a_file == true )
		send_response("200 OK", _content_type, _path);
	else if ( _method == "GET" )
	{

		if ( location.get_redirect() != "" )
		{
			redirection(_socket, location.get_redirect());
			return ;
		}

		std::string	content = location.get_root();
		if ( content == "" )
			content = _server->get_root();
		
		if ( location.get_allow_methods(GET) == false && _server->get_allow_methods(GET) == false )
		{
			if ( location.get_error_page(405) != "" )
				content += location.get_error_page(405);
			else
				content += _server->get_error_page(405);
			send_response("405 Method Not Allowed", "text/html", content);
		}
		else
		{
			if ( location.get_index() != "" )
				content += location.get_index();
			else
				content += _server->get_index();

			send_response("200 OK", "text/html", content);
		} 

	}
}

void loadFile(const std::string &fileName, std::stringstream &stream) {
	std::ifstream input_file(fileName.c_str());
	stream << input_file.rdbuf();
	input_file.close();
}

void	Request::send_image(std::string image, std::string response)
{
	std::cerr << "send_image" << std::endl;
	FILE	*img_file = fopen(image.c_str(), "rb");
	if ( img_file == NULL )
	{
		_request_a_file = false;
		send_response("404 not found", _content_type, "");
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
			ret -= send(_socket, response.c_str(), response.size(), 0);
		stream.close();
		delete[] buffer;
		std::cerr << "send_image debug b " << std::to_string(size) << " response: " <<  response << std::endl;
		buffer = nullptr;
	}
}

std::string	Request::error_handler(int status_code)
{
	std::string	file;
	std::string	content;

	file = _server->get_error_page(status_code);

	std::ifstream	content_stream(file.c_str());
	std::stringstream stream;

	stream << content_stream.rdbuf();
	content = stream.str();

	if ( content == "")
		content = "<h1>" + std::to_string(status_code) + "</h1>";

	return(content);
}

void	Request::send_response(std::string status_code, std::string content_type, std::string file)
{
	std::string response = "http/1.1 " + status_code + "\r\n";
	response += "Date: " + daytime() + "\r\n";
	response += "Connection: keep-alive\r\n";
	response += "Server: " + _server->get_server_name() + "\r\n";
	response += "Content-Type: " + content_type + "\r\n";

	std::cerr << "DEBUG" << std::endl;
	std::string content;
	if ( _request_a_file == true )
	{
		send_image(ASSETS_DIR + file, response);
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
		error_handler(400);
		
	response += "Content-Length: " + std::to_string(content.size()) + "\r\n\n";
	response += content + "\r\n";

	std::cerr << "[DEBUG] response sended:\n" << response << std::endl;
	size_t nbyte = response.size();
	while ( nbyte > 0 )
		nbyte -= send(_socket, response.c_str(), response.size(), 0);


}