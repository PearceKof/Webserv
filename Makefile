NAME = Webserv

CC = c++

FLAGS		= -Wall -Wextra -Werror -std=c++98

FILES		= ./src/Cluster.cpp ./src/Location.cpp ./src/Server.cpp ./src/Webserv.cpp ./src/Socket.cpp ./src/utils.cpp ./src/Request.cpp ./src/Mime.cpp
OBJ			= $(FILES:%.cpp=%.o)

LINUX_FILES = ./src/Cluster_linux.cpp ./src/Location.cpp ./src/Server.cpp ./src/Webserv.cpp ./src/Socket.cpp ./src/utils.cpp ./src/Request.cpp ./src/Mime.cpp
LINUX_OBJ	= $(LINUX_FILES:%.cpp=%.o)

all: $(OBJ) $(NAME)

linux : $(LINUX_OBJ)
	$(CC)  -o $(NAME) $(LINUX_OBJ)

$(NAME): $(OBJ)
	$(CC)  -o $(NAME) $(OBJ)

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f a.out
	rm -f $(OBJ)
	rm -f $(LINUX_OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

test:
	rm -f a.out
	$(CC) test.cpp
	./a.out localhost

relinux : fclean linux

.PHONY: all clean fclean re