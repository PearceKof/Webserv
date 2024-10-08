
#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "Webserv.hpp"
# include "Mime.hpp"
# include <vector>
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
		std::string							_host;
	
		std::string							_response;
		std::string							_header_response;
		std::string							_body_response;
		std::string							_status_code;
		std::string							_content_type;
		std::string							_content_lenght;

		Mime								_mime;
		Server*								_server;
		Cluster*							_cluster;
		std::string							_active_location;
		std::string							_cgi_path;
	
		int									_socket;
		int									_body_size;
		int									_left_to_read;
		int									_left_to_send;

		bool								_request_a_file;
		bool								_request_is_chunked;
		bool								_body_is_unfinished;
		bool								_max_body_size_reached;
		bool								_is_ready_to_send;

		void		set_path(std::map<std::string, Location> locations);
		void		generate_full_response();
		void		send_auto_index();
		void		redirection(std::string redirection);
		int			is_path_to_cgi(std::vector<std::string>& cgi_paths);
		bool		is_valid_cgi_extension();
		void		set_server();
		void 		parse_request();
		int 		read_body(ssize_t nbytes, char *buf);
		void	cgi();

		void		handle_GET();
		void		handle_POST();
		void		load_file();
		void		handle_DELETE();
		void		upload_file(std::string boundary);
		void		error(int status_code, std::string status_message);
		void		put_back_chunked();
	public:
		Request();
		~Request();
		void		set_fd_and_server(int fd, Server *server, Cluster &cluster);
		int			treat_received_data(char *buf, ssize_t nbytes);
		void		create_response();
		int			send_response();

		int&			get_socket(){ return _socket ; };
		std::string&	get_request() { return _request ; };
		std::string&	get_response() { return _response ; };
		std::string&	get_body_request() { return _body_request ; };
		std::string&	get_header_request(std::string index) { return _header_request[index] ; };
		bool			is_ready_to_send() {return _is_ready_to_send ; };
		void			set_ready(bool boolean) { _is_ready_to_send = boolean; };
};

#endif