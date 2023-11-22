
#include "Webserv.hpp"

bool	is_directory( std::string path )
{
	struct stat st;

	if ( stat(path.c_str(), &st) )
		return false ;

	return S_ISDIR(st.st_mode) ;
}

bool	is_existing_file( std::string path )
{
	std::ifstream ifs(path.c_str());
	bool is_open = ifs.is_open();

	if ( is_open )
		ifs.close();

	return is_open ;
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
	return std::string(buffer) ;
}