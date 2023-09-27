NAME = Webserv

CC = c++

FLAGS		= -Wall -Wextra -Werror -std=c++98

FILES		= ./src/Cluster.cpp ./src/Location.cpp ./src/Server.cpp ./src/Webserv.cpp ./src/Socket.cpp ./src/utils.cpp

OBJ			= $(FILES:%.cpp=%.o)

all: $(OBJ) $(NAME)

$(NAME): $(OBJ)
	$(CC)  -o $(NAME) $(OBJ)

%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re