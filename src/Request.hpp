
#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "Webserv.hpp"

struct client_info;
class Cluster;
class Location;

class Request
{
private:
	std::string	_method;
	std::string	_path;
	std::map<std::string , std::string>	_header;
	std::string	_body;

	Request();
	void	set_method(std::string request);
	void	set_path(std::string request, std::map<std::string, Location> locations);
	void	set_header_and_body(std::string request);
public:
	Request(int client_fd, client_info &client);
	~Request();

	void	handle_request(client_info client);
};

std::string	read_request(int client_socket);

#endif