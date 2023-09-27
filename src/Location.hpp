
#ifndef LOCATION_HPP
# define LOCATION_HPP

# include "webserv.hpp"
# include <map>

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

};

#endif