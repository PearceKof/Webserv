
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

std::string	read_request(int client_socket)
{
	std::string	request_str = "";
	char buffer[121];
	bzero(buffer, 121);

	int byte_readed = read(client_socket, buffer, 120);

	while ( 0 < byte_readed )
	{
		for ( int i = 0 ; i < byte_readed ; i++ )
			request_str.push_back(buffer[i]);

		byte_readed = read(client_socket, buffer, 120);
	}
	return request_str ;
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