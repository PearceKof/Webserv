
#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <string>
# include "webserv.hpp"
struct client_info;
class Cluster;
class Location;
class Request
{
private:
	std::string	_method;
	std::string	_path;
	std::string	_header;
	std::string	_body;

	Request();
	void	set_method(std::string request);
	void	set_path(std::string request, std::map<std::string, Location> locations);
	void	set_body(std::string request);
public:
	Request(int client_fd, client_info &client);
	~Request();
};

std::string	read_request(int client_socket);

#endif