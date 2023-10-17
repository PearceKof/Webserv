
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

// # include "Cluster_linux.hpp"
# include "Cluster.hpp"
# include <cstdlib>
# include <unistd.h>
# include <iostream>
# include <stdexcept>
# include <string>
# include <cstring>
#include <sys/event.h>
/*
------utils.cpp------
*/
std::string	trim_config(const char *to_find, std::string& server_config);

# define DEFAULT_CONFIG_PATH "./config/config_files/default.conf"

enum methods
{
	GET,
	POST,
	DELETE
};

#endif