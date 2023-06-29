# Lines starting with '#' are comments.

# Explicitly specify which files to compile
SOURCES = \
	main.c \
	utilities.c\

CFLAGS = -I../include/
LDFLAGS	= -L../libs/
LIBS	= -lgsl -lgslcblas
