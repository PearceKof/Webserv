
#include "Request.hpp"
#include <unistd.h>
#include <stdlib.h>

Request::Request()
{
	_body_is_unfinished = false;
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
	_content_lenght = "0";
	_active_location = "";
	_server = NULL;

}

void	Request::set_fd_and_server(int fd, Server *server, Cluster &cluster)
{
	_socket = fd;
	_server = server;
	_cluster = &cluster;
}

static	bool	is_valid_path(std::map<std::string, Location> locations, std::string path)
{
	for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; it++ )
		if ( it->first == path )
			return true ;

	return false ;
}

int	Request::is_path_to_cgi(std::vector<std::string>& cgi_paths)
{
	int index = 0;
	for ( std::vector<std::string>::iterator it = cgi_paths.begin() ; it != cgi_paths.end() ; it++ )
	{
		if ( _path.find(*it) != std::string::npos )
		{
			return index ;
		}
			
		index++;
	}
	return -1 ;
}

std::string decode_url(std::string coded_url)
{
	std::stringstream decoded_url;
	decoded_url << std::hex;
	decoded_url.fill(0);
	for ( size_t i = 0 ; coded_url[i] ; i++ )
	{
		if ( coded_url[i] == '%' && (i + 2) < coded_url.size() )
		{
			int value_hex;
			std::stringstream str_value_hex(coded_url.substr(i + 1, 2));
			str_value_hex >> std::hex >> value_hex;
			decoded_url << static_cast<unsigned char>(value_hex);
			i += 2;
		}
		else if (coded_url[i] == '+')
			decoded_url << ' ';
		else
			decoded_url << coded_url[i];
	}
	return decoded_url.str();
}

void	Request::set_path(std::map<std::string, Location> locations)
{
	size_t begin;
	size_t end;
	std::string extension = "default";

	std::cerr << "path before [" << _path << "]" << std::endl;
	_path = decode_url(_path);
	std::cerr << "path after [" << _path << "]" << std::endl;
	if ( _mime.is_a_file(_path) )
	{
		_request_a_file = true;
		begin = _path.find_last_of('.');
		end = _path.find(' ', begin);
		extension = _path.substr(begin, end - begin);
	}
	_content_type = _mime.get_content_type(extension);

	std::string root;
	for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; ++it )
	{
		
		// std::cerr << "[DEBUG]: locations[" << it->first << "].cgi_path=[" << locations[it->first].get_cgi_path() << "] && find(" << locations[it->first].get_cgi_path() << ") in [" << _path << "]" << std::endl;
		// if ( locations[it->first].get_cgi_path(0) != "" && _path.find(locations[it->first].get_cgi_path()) != std::string::npos )
		int index;
		if ( (index = is_path_to_cgi(locations[it->first].get_cgi_paths())) != -1 )
		{
			if ( locations[it->first].get_root() != "" )
				root = locations[it->first].get_root();
			else if ( _server->get_root() != "" )
				root = _server->get_root();
			else
				root = DEFAULT_ROOT;

			_cgi_path = root + locations[it->first].get_cgi_path(index);
			_active_location = it->first;
			std::cerr << "[DEBUG]: IS CGI PATH get_cgi_path[" << index << "] " << locations[it->first].get_cgi_path(index) << " _path="<< _path << " _path.find(locations[it->first].get_cgi_path())= " << _path.find(locations[it->first].get_cgi_path(index)) << std::endl;
			return ;
		}
		if ( (_path.find(it->first) == 0 && it->first != "/") || (_path == "/" && it->first == "/") )
		{
			std::string root;
			if ( locations[it->first].get_root() != "" )
				root = locations[it->first].get_root();
			else if ( _server->get_root() != "" )
				root = _server->get_root();
			else
				root = DEFAULT_ROOT;
			
			if ( _method != "DELETE" && locations[it->first].get_index() != "")
				_path = root + locations[it->first].get_index();
			else
				_path = _path.replace(_path.find(it->first), it->first.size(), root);
			// else
			// 	_path = root + _path;

			_active_location = it->first;
			// std::cerr << "[DEBUG]: It'S NOIT A CGI PATH" << "get_cgi_path=" << locations[it->first].get_cgi_path() << " _path="<< _path << " _path.find(locations[it->first].get_cgi_path())= " << _path.find(locations[it->first].get_cgi_path()) << std::endl;
			return ;
		}
	}
	_active_location = "/";
	if ( locations[_active_location].get_root() != "" )
		root = locations[_active_location].get_root();
	else if ( _server->get_root() != "" )
		root = _server->get_root();
	else
		root = DEFAULT_ROOT;
	_path = root + _path;
}

Request::~Request()
{
}

int Request::read_body( ssize_t nbytes, char *buf)
{
	if ( _left_to_read <= 0 && !_request_is_chunked )
	{
		return 0;
	}
	// std::cerr << "[DEBUG]: max body size: " <<  _server->get_client_max_body_size() << " current bodysize: " << (_body_request.size() + nbytes) << std::endl;
	if ( _server->get_client_max_body_size() != 0 && _server->get_client_max_body_size() < (_body_request.size() + nbytes) )
	{
		_max_body_size_reached = true;
		_left_to_read = 0;
		return 0;
	}

	for ( ssize_t i = 0 ; i < nbytes ; i++ )
		_body_request.push_back(buf[i]);
	
	if ( _left_to_read && _request_is_chunked == false )
	{
		_left_to_read -= nbytes;
		return _left_to_read > 0 ;
	}
	else if ( _request_is_chunked )
	{
		bool	end_of_file_reached = ( _body_request.find("\r\n0\r\n\r\n") != std::string::npos );
		return !end_of_file_reached ;
	}
	return 0 ;

}


void	Request::set_server()
{
	if ( _host == "" )
		return error(400, "Bad Request");
	std::string	hostname;
	std::string port;
	size_t col_pos = _host.find(':');
	if ( col_pos != std::string::npos )
	{
		hostname = _host.substr(0, _host.size() - (_host.size() - col_pos));
		col_pos++;
		port = _host.substr(col_pos, _host.size() - col_pos);
	}
	else
	{
		hostname = _host;
		port = "80";
	}

	int index = 0;
	std::vector<Server> list_of_serv = _cluster->get_servers();
	for ( std::vector<Server>::iterator it_serv = list_of_serv.begin() ; it_serv !=list_of_serv.end() ; it_serv++ )
	{
		std::vector<std::pair<std::string, int> > list_of_host = it_serv->get_listening_port();
		for ( std::vector<std::pair<std::string, int> >::iterator it_host = list_of_host.begin() ; it_host != list_of_host.end() ; it_host++)
		{
			// std::cerr << "[DEBUG]: host:" << hostname << " compared to " << it_host->first << " port:" << port << " compared to " << it_host->second << std::endl; 
			if ( (hostname == it_host->first || hostname == it_serv->get_server_name()) && atoi(port.c_str()) == it_host->second )
			{
				_server = _cluster->get_server(index);
				return ;
			}
		}
		index++;
	}
}

void	Request::parse_request()
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

	if ( _header_request.find("Host") != _header_request.end() )
	{
		_host = _header_request["Host"];
	}

	set_server();

	_left_to_read = _body_size;
}

int	Request::treat_received_data(char *buf, ssize_t nbytes)
{
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

	while ( std::getline(ss, line) )
	{
		long chunk_size = std::strtol(line.c_str(), NULL, 16);
		if ( chunk_size == 0 )
			break ;
		std::string	chunk;

		chunk.resize(chunk_size);
		ss.read(&chunk[0], chunk_size);
		// ss.ignore(2);
		_body_response += chunk.data();
	}
	// _body_response += line.data();
	_body_request.clear();
	_body_request = _body_response;
}

void	Request::cgi()
{
	std::string query_string;
	std::string method_env = "REQUEST_METHOD=" + _method;
	if ( access(_cgi_path.c_str(), X_OK) )
		return error(403, "Forbidden");
	if(_method == "GET" && _server->get_locations()[_active_location].get_allow_methods(GET))
	{
		query_string = _path.substr(_path.find("?") + 1);
	}
	else if (_method == "POST" && _server->get_locations()[_active_location].get_allow_methods(POST))
	{
		query_string = _body_request;
	}
	else
		return error(405, "Method Not Allowed");

	std::string bodyFromScript;
	int	pipe_fd[2];

	if(pipe(pipe_fd) == -1)
		return error(500, "Internal Server Error");
	
	pid_t pid = fork();
	if(pid == -1)
		return error(500, "Internal Server Error");
	else if(pid == 0)
	{
		const char	*pythonExecutable = "/usr/bin/python3";
		char	*argv[] = {(char*)pythonExecutable, (char*)_cgi_path.c_str(), nullptr};
		char *envp[] = {(char*)query_string.c_str(), (char*)method_env.c_str(), nullptr};
		close(pipe_fd[0]);
		dup2(pipe_fd[1], STDOUT_FILENO);
		close(pipe_fd[1]);
		
		if(execve(pythonExecutable, argv, envp) == -1)
			exit(EXIT_FAILURE);
	}
	else
	{
		int status;
		waitpid(pid, &status, 0);
		
		if (WIFEXITED(status) && status == 0)
		{
			close(pipe_fd[1]);

			char buffer[1024];
			ssize_t n;
			while ((n = read(pipe_fd[0], buffer, 1024)) > 0)
				_body_response.append(buffer, n);
			close(pipe_fd[0]);
			waitpid(-1, NULL, 0);
		}
		else
			return error(500, "Internal Server Error");
	}
}

bool	Request::is_valid_cgi_extension()
{
	size_t ext_pos = _cgi_path.find_last_of(".");
	size_t i = 0;

	if ( ext_pos != std::string::npos )
	{
		ext_pos++;
		std::string		ext = _cgi_path.substr(ext_pos, _cgi_path.size() - ext_pos );
		for ( i = 0 ; i < _server->get_locations()[_active_location].get_cgi_extension_size() ; i++ )
		{
			if ( ext == _server->get_locations()[_active_location].get_cgi_extension(i) )
				return true ;
		}
	}
	return false ;
}

void	Request::create_response()
{
	if ( _host == "")
	{
		return error(400, "Bad Request");
	}
	set_path(_server->get_locations());

	if ( _request_is_chunked )
		put_back_chunked();

	if ( _method.empty() || _path.empty() || _version.empty() )
	{
		error(400, "Bad Request");
	}
	else if ( _max_body_size_reached || _body_request.size() > _server->get_client_max_body_size())
	{
		error(413, "Content Too Large");
	}
	else if ( _version != "HTTP/1.1")
	{
		error(505, "HTTP Version Not Supported");
	}
	else if ( _server->get_locations()[_active_location].get_redirect() != "" )
	{
		redirection(_server->get_locations()[_active_location].get_redirect());
	}
	else if ( _cgi_path != "" )
	{
		if ( is_valid_cgi_extension() )
			cgi();
		else
			error(415, "Unsupported Media Type");
	}
	else
	{
		if ( _method == "GET" && _server->get_locations()[_active_location].get_allow_methods(GET))
			handle_GET();
		else if ( _method == "POST"  && _server->get_locations()[_active_location].get_allow_methods(POST))
		{
			if ( _server->get_locations()[_active_location].get_upload() == true )
				handle_POST();
			else
				error(403, "Forbidden");
		}
		else if ( _method == "DELETE" && _server->get_locations()[_active_location].get_allow_methods(DELETE) )
			handle_DELETE();
		else
			error(405, "Method Not Allowed");
	}

	generate_full_response();
}


void	Request::redirection(std::string redirection)
{
	_status_code = "301 Moved Permanently";
	_header_response += "Location: " + redirection + "\r\n";
}

void	Request::send_auto_index()
{
	_status_code = "200 OK";

	DIR *dir = opendir(_path.c_str());
	if ( dir == NULL )
		return error(403, "Forbidden");

	_body_response = "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><table>";
	_body_response += "<tr><td><a href=\"../\">../</a></td><td>-</td></tr>";

	struct dirent *dirent;

	while ( (dirent = readdir(dir)) != NULL )
	{
		std::string name = dirent->d_name;
		std::string path =  _active_location + "/" + name;

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
			_body_response += "<tr><td><a href=\"" + path + "\">" + name + "</a></td><td>" + size + "</td></tr>";
	}
	_body_response += "</table></body></html>";
	closedir(dir);
	_content_lenght = std::to_string(_body_response.size());
}

void	Request::load_file()
{
	std::ifstream ifs(_path.c_str());
	std::stringstream ss;

	ss << ifs.rdbuf();
	ifs.close();
	_body_response = ss.str();
}

void	Request::handle_GET()
{
	_status_code = "200 OK";
	Location location = _server->get_locations()[_active_location];

	if ( is_directory(_path) )
	{
		if ( location.get_auto_index() == true )
			send_auto_index();
		else
		{
			if ( location.get_index() == "" )
				return error(404, "Not Found");
			
			_path += "/" + location.get_index();
			if ( is_existing_file( _path ) )
				load_file();
			else
				return error(404, "Not Found");
		}
	}
	else if ( is_existing_file(_path) )
		load_file();
	else
		return error(404, "Not Found");
}

std::string	get_filename(std::string body)
{
	size_t start_filename = body.find("filename=\"");
	if (start_filename == std::string::npos)
		return "" ;
	size_t end_filename = body.find('\"', start_filename + 10);
	if (end_filename == std::string::npos)
		return "" ;

	return body.substr(start_filename + 10, end_filename - (start_filename + 10)) ;
}

void	Request::upload_file(std::string boundary)
{
	Location active_location = _server->get_locations()[_path];
	std::string filename = get_filename(_body_request);
	if ( filename.empty() )
		return error(400, "Bad Request");
	size_t	begin = _body_request.find("\r\n\r\n");
	size_t	end = _body_request.find("\r\n--" + boundary + "--", begin);
	if ( begin == std::string::npos || end == std::string::npos )
		return error(400, "Bad Request");
	std::string	body_trimmed = _body_request.substr(begin + 4, end - (begin + 4));

	std::string path;
	if ( active_location.get_upload_path() != "" )
		path = active_location.get_upload_path() + "/" + filename;
	else
		path = DEFAULT_UPLOAD_PATH"/" + filename;

	if ( !access(path.c_str(), F_OK) )
		return error(409, "Conflict");
	else
	{
		std::ofstream ofs(path);
		ofs.write(body_trimmed.c_str(), body_trimmed.size());
		ofs.close();
	}
	_status_code = "201 Created";
	_body_response = body_trimmed;
	_content_type = _mime.get_content_type(filename);
	_content_lenght = std::to_string(_body_response.size());
	_header_response += "Location: " + path + "\r\n";
}

void	Request::handle_POST()
{
	_status_code = "200 OK";

	size_t pos_boundary = _header_request["Content-Type"].find("boundary=");
	if (pos_boundary != std::string::npos)
	{
		upload_file(_header_request["Content-Type"].substr(pos_boundary + 9));
	}
	else if ( _header_request["Content-Type"].find("application/x-www-form-urlencoded") != std::string::npos )
	{
		_body_response = "application/x-www-form-urlencoded";
	}
	else if ( _header_request["Content-Type"].find("plain/text") != std::string::npos )
	{

	}
	else
		error(501, "Not Implemented");
}


void	Request::handle_DELETE()
{
	_status_code = "204 No Content";
	
	std::cerr << "[DEBUG]: DELETE _path= " << _path << " is existing file: " << is_existing_file(_path) << " active_location=" << _active_location  << std::endl;
	if ( is_existing_file(_path) == false )
	{
		return error(404, "Not Found");
	}
	if ( _path == _active_location || std::remove(_path.c_str()) )
		return error(403, "Forbidden");
}

void	Request::generate_full_response()
{
	_header_response += "Date: " + daytime() + "\r\n";
	if ( _host != "")
		_header_response += "Host: " + _host + "\r\n";
	if ( _server->get_server_name() != "")
		_header_response += "Server: " + _server->get_server_name() + "\r\n";
	if ( _content_type != "")
		_header_response += "Content-Type: " + _content_type + "\r\n";
	_header_response += "Content-Length: " + std::to_string(_body_response.size()) + "\r\n";

	_response = "HTTP/1.1 " + _status_code + "\r\n";
	_response += _header_response;
	_response += "\r\n";

	_response += _body_response;

	_left_to_send = _response.size();
	std::cerr << "[DEBUG]: response generated for client[" << _socket << "]:\n[" << _response << "]\nleft_to_send=" << _left_to_send << std::endl;
}

int	Request::send_response()
{
	ssize_t nbytes;

	nbytes = send(_socket, _response.c_str(), 120, 0);
	if (nbytes == -1 )
	{
		std::cerr << "[ERROR]: client [" << _socket << "] failed to send response" << std::endl;
		return -1 ;
	}
	_response.erase(0, nbytes);

	_left_to_send -= nbytes;
	if ( _left_to_send <= 0 || nbytes == 0 )
	{
		std::cout << "[WEBSERV]: sended full response to client [" << _socket << "] successfully" << std::endl;
		return 1;
	}

	return 0 ;
}

void	Request::error(int status_code, std::string status_message)
{
	std::string	file;

	if ( _server )
	{
		file = _server->get_root() + _server->get_error_page(status_code);
		std::ifstream	content_stream(file.c_str());
		std::stringstream stream;

		stream << content_stream.rdbuf();
		_body_response = stream.str();

		_status_code = std::to_string(status_code);
	}
	if ( status_message != "" )
		_status_code += " " + status_message;


	if ( _body_response == "")
		_body_response = "<h1>ERROR: " + _status_code + "</h1>";

	_content_type = "text/html";

}