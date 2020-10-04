CC = gcc
SOURCES = server.c function.c
EXECUTABLE = server
OPTION = -O2

$(EXECUTABLE): $(SOURCES)
	$(CC) $(SOURCES) -o $(EXECUTABLE) $(OPTION)

clean:
	rm $(EXECUTABLE)
