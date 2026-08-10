NAME		= codexion

CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread
INCLUDES	= -Iincludes

SRCS_DIR	= srcs
SRCS		= \
	$(SRCS_DIR)/main.c \
	$(SRCS_DIR)/parse_args.c \
	$(SRCS_DIR)/parse_number.c \
	$(SRCS_DIR)/util.c \
	$(SRCS_DIR)/time_utils.c \
	$(SRCS_DIR)/init_sim.c \
	$(SRCS_DIR)/init_dongles.c \
	$(SRCS_DIR)/init_coders.c \
	$(SRCS_DIR)/heap.c \
	$(SRCS_DIR)/heap_sift.c \
	$(SRCS_DIR)/dongle_take.c \
	$(SRCS_DIR)/dongle_release.c \
	$(SRCS_DIR)/coder_routine.c \
	$(SRCS_DIR)/coder_compile.c \
	$(SRCS_DIR)/coder_rest.c \
	$(SRCS_DIR)/monitor.c \
	$(SRCS_DIR)/logger.c \
	$(SRCS_DIR)/sim_run.c \
	$(SRCS_DIR)/cleanup.c

OBJS		= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(SRCS_DIR)/%.o: $(SRCS_DIR)/%.c includes/codexion.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
