NAME		= ft_nm

CC			= cc
SRCDIR		= src
OBJDIR		= obj

MODULES		= elf_stream \
			  symbol_table \
			  nm_display

SRCS		= $(SRCDIR)/main.c \
			  $(foreach m,$(MODULES),$(SRCDIR)/$(m)/$(m).c)
OBJS		= $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS		= $(OBJS:.o=.d)

IFLAGS		= -I$(SRCDIR) $(foreach m,$(MODULES),-I$(SRCDIR)/$(m))
CFLAGS		= -Wall -Wextra -Werror -std=c23 $(IFLAGS)
DEPFLAGS	= -MMD -MP

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean
	$(MAKE) all

.PHONY: all clean fclean re
