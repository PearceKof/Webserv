
#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "Webserv.hpp"
# include "Mime.hpp"
# include <string>
# include <map>
# include <dirent.h>
# include <sys/stat.h>
// # include <sys/type.h>

// struct client_info;
class Mime;
class Cluster;
class Location;
class Server;

class Request
{
	private:
		std::string							_request;
		std::map<std::string , std::string>	_header_request;
		std::string							_body_request;
		std::string							_method;
		std::string							_path;
		std::string							_version;
	
		std::string							_response;
		std::string							_header_response;
		std::string							_body_response;
		std::string							_status_code;
		std::string							_content_type;
		std::string							_content_lenght;

		Mime								_mime;
		Server*								_server;
	
		int									_socket;
		int									_body_size;
		int									_left_to_read;
		int									_left_to_send;

		bool								_request_a_file;
		bool								_request_is_chunked;
		bool								_body_is_unfinished;
		bool								_max_body_size_reached;

		void		set_path(std::map<std::string, Location> locations);
		// void		make_response( std::string status_code, std::string file);
		void		generate_full_response();
		// std::string	get_image(std::string image);
		void		send_auto_index();
		void		redirection(std::string redirection);
	public:
		Request();
		~Request();
		void		set_fd_and_server(int fd, Server *server);
		int			treat_received_data(char *buf, ssize_t nbytes);
		int 		read_body(ssize_t nbytes, char *buf);
		void 		parse_request();
		void		create_response();
		void		put_back_chunked();
		bool		send_response();
		void		load_file();

		void		handle_GET();
		void		error(int status_code);

		std::string&	get_request() { return _request ; };
		std::string&	get_response() { return _response ; };
		std::string&	get_body_request() { return _body_request ; };
		std::string&	get_header_request(std::string index) { return _header_request[index] ; };
};

#endif