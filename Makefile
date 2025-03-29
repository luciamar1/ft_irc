NAME = ircserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -Ilib
DEBUG_FLAGS = -g  # Agrega la flag de depuración

SRC_DIR = src
OBJ_DIR = obj
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

# Variable que indica si estamos en modo de depuración
DEBUG = 0

# Si DEBUG está activado, añadimos las flags de depuración
ifeq ($(DEBUG), 1)
    CFLAGS += $(DEBUG_FLAGS)
endif

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Modo de depuración
debug: DEBUG = 1
debug: re

.PHONY: all clean fclean re debug
