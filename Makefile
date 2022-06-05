
APP = tdd

all: 
	scons

clean:
	rm -r $(ODIR) -f $(APP) html latex
	scons -c

test: $(APP)
	./tdd 

doxygen: $(APP)
	doxygen

valgrind: $(APP)
	valgrind --leak-check=full ./$(APP)

# FIN
