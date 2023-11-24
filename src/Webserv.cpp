
#include "Webserv.hpp"

int run = 1;

void	sigint_handler(int sig)
{
	run = 0;
}

int	main(int ac, char **av)
{
	if ( 2 < ac )
	{
		std::cerr << "[ERROR]: Bad arguments" << std::endl;
		return EXIT_FAILURE ;
	}

	Cluster	cluster;

	try
	{
		if (ac == 2)
			cluster.config(av[1]);
		else
			cluster.config(DEFAULT_CONFIG_PATH);
		std::signal(SIGINT, sigint_handler);
		if ( cluster.is_valid_config() == false )
		{
			std::cerr << "[ERROR]: Invalid configuration file" << std::endl;
			return EXIT_FAILURE ;
		}
		cluster.print_all();
		cluster.run();
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
	}

	return EXIT_SUCCESS ;
}