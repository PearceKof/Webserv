
#ifndef WEBSERV_HPP
# define WEBSERV_HPP

# define ASSETS_DIR "assets"
# define DEFAULT_CONFIG_PATH "config/default.conf"
# define DEFAULT_UPLOAD_PATH "www/upload"
# define DEFAULT_ROOT "www"

# include "Cluster.hpp"
# include "Socket.hpp"
# include "Server.hpp"
# include "Request.hpp"
# include "Location.hpp"
# include <sys/wait.h>
# include <csignal>
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
bool	is_directory(std::string path);
bool	is_existing_file( std::string path );
std::string	trim_config(const char *to_find, std::string& server_config);
std::string	daytime();

enum methods
{
	GET,
	POST,
	DELETE
};

#endif