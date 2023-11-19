
#include "Request.hpp"
#include <unistd.h>

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
	size_t begin;
	size_t end;
	std::string extension = "default";

	if ( _mime.is_a_file(_request) )
	{
		_request_a_file = true;
		begin = _path.find_last_of('.');
		end = _path.find(' ', begin);
		extension = _path.substr(begin, end - begin);
	}
	_content_type = _mime.get_content_type(extension);

	for ( std::map<std::string, Location>::iterator it = locations.begin() ; it != locations.end() ; ++it )
	{
		if ( _path == it->first || (_path == "/" && it->first == "/") )
		{
			std::string root;
			if ( locations[it->first].get_root() != "" )
				root = locations[it->first].get_root();
			else if ( _server->get_root() != "" )
				root = _server->get_root();
			else
				root = DEFAULT_ROOT;

			if (locations[it->first].get_index() != "")
				_path = root + locations[it->first].get_index();

			_active_location = it->first;
			return ;
		}
		if (locations[it->first].get_cgi_path() != "" && _path.find(locations[it->first].get_cgi_path()) != std::string::npos)
		{
			std::string root;
			if ( locations[it->first].get_root() != "" )
				root = locations[it->first].get_root();
			else if ( _server->get_root() != "" )
				root = _server->get_root();
			else
				root = DEFAULT_ROOT;

			_cgi_path = root + locations[it->first].get_cgi_path();
			return ;
		}
	}
	_path = DEFAULT_ROOT + _path;
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
		std::cerr << "buf=[" << buf << "] left_to_read: " << _left_to_read << "nbytes= " << nbytes << std::endl;
		return _left_to_read > 0 ;
	}
	else if ( _request_is_chunked )
	{
		bool	end_of_file_reached = ( _body_request.find("\r\n0\r\n\r\n") != std::string::npos );
		return !end_of_file_reached ;
	}
	return 0 ;

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

	std::cerr << "[DEBUG]: THIS IS A CHUNKED REQUEST !!!" << std::endl;
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
		std::cerr << "[DEBUG]: chunk=[" << chunk << "]" << std::endl;
	}
	std::cerr << "[DEBUG]: end main loop" << std::endl;
	// _body_response += line.data();
	_body_request.clear();
	_body_request = _body_response;
}

void	Request::create_response()
{
	set_path(_server->get_locations());

	if ( _request_is_chunked )
		put_back_chunked();

	if ( _method.empty() || _path.empty() || _version.empty() )
	{
		error(400, "Bad Request");
	}
	else if ( _max_body_size_reached )
	{
		error(413, "Unauthorized");
	}
	else if ( _version != "HTTP/1.1")
	{
		error(505, "HTTP Version Not Supported");
	}
	else if ( _cgi_path != "")
	{
		std::string bodyFromScript;
		int	pipe_fd[2];
		
		const char	*pythonExecutable = "/usr/bin/python3";
		char	*argv[] = {(char*)pythonExecutable, (char*)_cgi_path.c_str(), nullptr};
		
		if(pipe(pipe_fd) == -1)
			perror("pipe");
		pid_t pid = fork();
		if(pid == -1)
		{
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if(pid == 0)
		{
			std::string query_string = _path.substr(_path.find("?") + 1);
			std::cerr << "Query String = " << query_string << std::endl;
			char *envp[] = {(char*)query_string.c_str(),
						(char*)"REQUEST_METHOD=GET",
						nullptr};
			
			close(pipe_fd[0]);
			dup2(pipe_fd[1], STDOUT_FILENO);
			close(pipe_fd[1]);
			
			if(execve(pythonExecutable, argv, envp) == -1)
			{
				perror("execve");
				exit(EXIT_FAILURE);
			}
		}
		else
		{
			int status;
			waitpid(pid, &status, 0);
			
			if (WIFEXITED(status))
			{
				//int exitCode = WEXITSTATUS(status);
				//std::cout << "Le script Python s'est terminé avec le code de sortie : " << exitCode << std::endl;
				close(pipe_fd[1]);

				char buffer[1024];
				ssize_t n;
				while ((n = read(pipe_fd[0], buffer, 1024)) > 0)
					bodyFromScript.append(buffer, n);
				close(pipe_fd[0]);
				waitpid(-1, NULL, 0);
				std::cout << "-------- BODY FROM SCRIPT --------\n" << bodyFromScript << std::endl;
				_body_response.append(bodyFromScript);
			}
			else
			{
				std::cerr << "Le script Python ne s'est pas terminé normalement." << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}
	else
	{
		if ( _method == "GET" && _server->get_locations()[_active_location].get_allow_methods(GET))
			handle_GET();
		else if ( _method == "POST" && _server->get_locations()[_active_location].get_allow_methods(POST))
			handle_POST();
		else if ( _method == "DELETE" && _server->get_locations()[_active_location].get_allow_methods(DELETE) )
			handle_DELETE();
		else
		{
			// std::cerr << "[DEBUG]: _method =[" << _method << "]" << std::endl;
			// if ( _method == "GET" ||  _method == "POST" || _method == "DELETE" )
				error(405, "Method Not Allowed");
			// else
			// 	error(501, "Not Implemented");
		}
	}

	generate_full_response();
}

void	Request::generate_full_response()
{
	_header_response += "Date: " + daytime() + "\r\n";
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

void	Request::redirection(std::string redirection)
{
	_status_code = "301 Moved Permanently";
	_header_response += "Location: " + redirection + "\r\n";
	_header_response += "Content-Length: 0\r\n\r\n";
}

void	Request::send_auto_index()
{
	_status_code = "200 OK";
	_header_response += "Date: " + daytime() + "\r\n";
	_header_response += "Server: " + _server->get_server_name() + "\r\n";
	_header_response += "Content-Type: text/html\r\n";
	_path = "." + _path;
	DIR *dir = opendir(_path.c_str());
	if ( dir == NULL )
	{
		error(403, "Forbidden");
		return ;
	}

	_body_response = "<html><head><title>Directory Listing</title></head><body><h1>Directory Listing</h1><table>";
	_body_response += "<tr><td><a href=\"../\">../</a></td><td>-</td></tr>";

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
			_body_response += "<tr><td><a href=\"" + _path + "/" + name + "\">" + name + "</a></td><td>" + size + "</td></tr>";
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
	Location location = _server->get_locations()[_path];

	if ( is_directory(_path) )
	{
		std::cerr << "\n" << _path << " IS A DIRECTORY\n" << std::endl;

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
	else if ( is_existing_file(  _path ) )
	{
		load_file();
	}
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
	else if ( _server->get_upload_path() != "" )
		path = _server->get_upload_path() + "/" + filename;
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
	_header_response += "Location: " + path;
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
		std::cerr << "\n\n[DEBUG] application/x-www-form-urlencoded\n\n" << std::endl;
		_body_response = "response to application/x-www-form-urlencoded";
	}
	else
		error(501, "Not Implemented");
}


void	Request::handle_DELETE()
{
	_status_code = "200 OK";
	
	_path = "." + _path;
	if ( is_existing_file(_path) )
	{
		return error(404, "Not Found");
	}
	if ( std::remove(_path.c_str()) )
		return error(403, "Forbidden");
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
	if ( _left_to_send <= 0 )
	{
		std::cout << "[WEBSERV]: sended full response to client	[" << _socket << "] successfully" << std::endl;
		return 1;
	}

	return 0 ;
}

void	Request::error(int status_code, std::string status_message)
{
	std::string	file;

	file = _server->get_root() + _server->get_error_page(status_code);

	std::ifstream	content_stream(file.c_str());
	std::stringstream stream;

	stream << content_stream.rdbuf();
	_body_response = stream.str();

	_status_code = std::to_string(status_code);
	if ( status_message != "" )
		_status_code += " " + status_message;

	if ( _body_response == "")
		_body_response = "<h1>ERROR: " + _status_code + "</h1>";

	_content_type = "text/html";

}