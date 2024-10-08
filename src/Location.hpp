
#ifndef LOCATION_HPP
# define LOCATION_HPP

# include "Webserv.hpp"

#include <map>
#include <string>
#include <vector>

class Location
{
	private:
		std::map<int, std::string>	_error_pages;
		std::string					_root;
		std::string					_index;
		std::string					_redirect;
		std::string					_upload_path;
		std::vector<std::string>	_cgi_path;
		std::vector<std::string>	_cgi_extension;
		bool						_allow_methods[3];
		bool						_auto_index;
		bool						_upload;
		bool						_is_directory;

		void	set_attributs(std::string& location_config);
		void	set_error_pages(std::string location_config);
		void	set_allow_methods(std::string location_config);
		void	set_cgi_path(std::string location_config);
		void	set_cgi_extension(std::string location_config);

	public:
		Location();
		Location(std::string location_config, std::string location_name);
		~Location();

		std::string	get_root() { return _root ; };
		// std::string	get_server_name() { return _server_name ; };
		std::string	get_index() { return _index ; };
		std::string	get_redirect() { return _redirect ; };
		std::string	get_upload_path() { return _upload_path ; };
		std::string	get_cgi_path(int index) { return _cgi_path[index] ; };
		size_t		get_cgi_path_size() { return _cgi_path.size(); };
		std::vector<std::string>&	get_cgi_paths() { return _cgi_path ; };
		std::string	get_cgi_extension(int index) { return _cgi_extension[index] ; };
		size_t		get_cgi_extension_size() { return _cgi_extension.size(); };
		bool		get_allow_methods(int index) { return _allow_methods[index] ; };
		bool		get_auto_index() { return _auto_index ; };
		bool		get_upload() { return _upload ; };
		std::string	get_error_page(int index) { return _error_pages[index] ; };
		bool		is_directory() { return _is_directory ; };
};

#endif