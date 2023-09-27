
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# include "Cluster.hpp"
# include <cstdlib>
# include <iostream>
# include <stdexcept>
# include <string>
# include <cstring>
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