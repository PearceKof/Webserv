
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

// # include "Cluster_linux.hpp"
# include "Cluster.hpp"
# include "Socket.hpp"
# include "Server.hpp"
# include "Request.hpp"
# include "Location.hpp"

# include <cstdlib>
# include <iostream>
# include <stdexcept>
# include <cstring>

# include <fcntl.h>
# include <unistd.h>
# include <sys/event.h>
# include <string>
# include <map>
/*
------utils.cpp------
*/
std::string	trim_config(const char *to_find, std::string& server_config);
std::string	daytime();
# define DEFAULT_CONFIG_PATH "./config/config_files/default.conf"

enum methods
{
	GET,
	POST,
	DELETE
};

#endif