# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ncharret <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2015/01/05 19:16:41 by ncharret          #+#    #+#              #
#    Updated: 2016/11/26 20:03:02 by ncharret         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #
NAME = particle_system
SRC = srcs/application.cpp srcs/glutils.cpp srcs/main.cpp srcs/utils.cpp
HEADERS = -I ~/.brew/Cellar/glfw3/3.2.1/include/
OBJ = $(SRC:.cpp=.o)
GLFW = ./lib/libglfw3.3.2.dylib

all: $(NAME)

$(NAME): $(OBJ)
	clang++ -stdlib=libc++ -std=c++11 $(HEADERS) -o $(NAME) $(GLFW) $(OBJ) $(HEADERS) -F/System/Library/Frameworks -framework OpenCL -framework OpenGL
	@echo "\\033[1;32m$(NAME) was created."

%.o: %.cpp
	@clang++ -stdlib=libc++ -std=c++11 $(HEADERS) -Wall -Werror -Wextra -Wno-unused-parameter -o $@ -c $^ $(HEADERS)

clean:
	@rm -f $(OBJ)

fclean: clean
	@rm -f $(NAME)

re: fclean all
