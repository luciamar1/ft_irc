
NAME = ircserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98
SRC = main.cpp Server.cpp Client.cpp Channel.cpp
OBJ = $(SRC:.cpp=.o)
HEADER = Server.hpp Client.hpp Channel.hpp

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

%.o: %.cpp $(HEADER)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all