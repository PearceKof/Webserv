
#ifndef LOCATION_HPP
# define LOCATION_HPP

# include "Webserv.hpp"

class Location
{
private:
	std::map<int, std::string>	_error_pages;
	std::string	_root;
	std::string	_server_name;
	std::string	_index;
	std::string	_redirect;
	std::string	_upload_path;
	std::string	_cgi_path;
	std::string	_client_max_body_size;
	bool	_allow_methods[3];
	bool	_auto_index;
	bool	_upload;

	void	set_attributs(std::string& location_config);
	void	set_error_pages(std::string location_config);
	void	set_allow_methods(std::string location_config);
public:
	Location();
	Location(std::string location_config);
	~Location();

	std::string	get_root() { return _root ; };
	std::string	get_server_name() { return _server_name ; };
	std::string	get_index() { return _index ; };
	std::string	get_redirect() { return _redirect ; };
	std::string	get_upload_path() { return _upload_path ; };
	std::string	get_cgi_path() { return _cgi_path ; };
	bool		get_allow_methods(int index) { return _allow_methods[index] ; };
	bool		get_auto_index() { return _auto_index ; };
	bool		get_upload() { return _upload ; };
	std::string	get_client_max_body_size() { return _client_max_body_size ; };
	std::string	get_error_page(int index) { return _error_pages[index] ; };
};

#endif