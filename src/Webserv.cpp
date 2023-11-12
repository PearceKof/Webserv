
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
		if ( cluster.is_valid_config() == false )
			return EXIT_FAILURE ;
		cluster.run();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}

	return EXIT_SUCCESS ;
}