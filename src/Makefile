PROG = serial
OBJDIR = .

CFLAGS += -std=gnu99 -W -Wall -pedantic
SOURCES = main.c io_posix.c io_winapi.c
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean
all: $(OBJDIR)/$(PROG)

clean:
	-rm -f $(OBJS) $(OBJDIR)/$(PROG)

$(OBJDIR)/$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
