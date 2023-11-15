
#ifndef SERVER_HPP
# define SERVER_HPP

# include "Webserv.hpp"
# include <vector>
# include <map>
# include <string>

class Location;

class Server
{
	private:
		std::map<std::string, Location>				_locations;
		std::map<int, std::string>					_error_pages;
		std::vector<std::pair<std::string, int> >	_listening_port;
		std::vector<std::string>		_cgi_extension;
		std::string	_root;
		std::string	_server_name;
		std::string	_index;
		std::string	_redirect;
		std::string	_upload_path;
		std::string	_cgi_path;
		bool	_allow_methods[3];
		bool	_auto_index;
		bool	_upload;
		int		_client_max_body_size;

		void	set_locations(std::string& server_config);
		void	set_attributs(std::string& server_config);
		void	set_error_pages(std::string server_config);
		void	set_allow_methods(std::string server_config);
		void	set_cgi_extension(std::string server_config);
		void	set_listen(std::string location_config);
		Server();

	public:
		Server(std::string server_config);
		~Server();

		std::map<std::string, Location>	get_locations() { return _locations ; };
		std::vector<std::pair<std::string, int> >	get_listening_port() { return _listening_port ; };
		std::string	get_cgi_extension(int index) { return _cgi_extension[index] ; };
		size_t		get_cgi_extension_size() { return _cgi_extension.size(); };
		std::string	get_root() { return _root ; };
		std::string	get_server_name() { return _server_name ; };
		std::string	get_index() { return _index ; };
		std::string	get_redirect() { return _redirect ; };
		std::string	get_upload_path() { return _upload_path ; };
		std::string	get_cgi_path() { return _cgi_path ; };
		bool		get_allow_methods(int index) { return _allow_methods[index] ; };
		bool		get_auto_index() { return _auto_index ; };
		bool		get_upload() { return _upload ; };
		int			get_client_max_body_size() { return _client_max_body_size ; };
		std::string	get_error_page(int index) { return _error_pages[index] ; };
		bool		is_valid_server();
};

#endif