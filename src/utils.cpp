
#include "Webserv.hpp"

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