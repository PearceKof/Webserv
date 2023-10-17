
#include "Webserv.hpp"

int	main(int ac, char **av)
{
	if ( ac != 2 )
	{
		std::cerr << "Bad arguments" << std::endl;
		return EXIT_FAILURE ;
	}

	Cluster	cluster;

	try
	{
		cluster.config(av[1]);
		cluster.print_all();
		cluster.setup();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}

	return EXIT_SUCCESS ;
}