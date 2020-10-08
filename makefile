CC = gcc
SOURCES = server.c function.c
EXECUTABLE = server
OPTION = -O2 -w
PTHREAD = -lpthread

$(EXECUTABLE): $(SOURCES)
	$(CC) $(SOURCES) -o $(EXECUTABLE) $(OPTION) $(PTHREAD)

clean:
	rm $(EXECUTABLE)
