CC = gcc
SOURCES = server.c function.c
EXECUTABLE = server
OPTION = -O2
PTHREAD = -lpthread

$(EXECUTABLE): $(SOURCES)
	$(CC) $(SOURCES) -o $(EXECUTABLE) $(OPTION) $(PTHREAD)

clean:
	rm $(EXECUTABLE)
