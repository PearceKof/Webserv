
#include "Webserv.hpp"

bool	is_directory( std::string path )
{
	struct stat buf;

	if ( path == "/")
		return false ;

	path = "." + path;
	if ( stat(path.c_str(), &buf) )
		return false ;

	return true ;
}

std::string	trim_config(const char *to_find, std::string& server_config)
{
	size_t	begin;
	size_t	end;
	size_t	size = strlen(to_find) + 1;

	begin = server_config.find(to_find) + size;
	if ( begin != std::string::npos + size )
	{
		end = server_config.find('\n', begin);
		return server_config.substr(begin, end - begin) ;
	}

	return "" ;
}

std::string	daytime()
{
	time_t		timer;
	struct tm	*timeinfo;
	char		buffer[80];

	time(&timer);
	timeinfo = localtime(&timer);
	strftime(buffer, 80, "%a %b %d %H:%M:%S %Y", timeinfo);
	// std::string	time(buffer);
	return std::string(buffer) ;
}