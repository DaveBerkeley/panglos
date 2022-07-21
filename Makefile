
APP = tdd

$(APP): 
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

clang: export CC=clang
clang: export CXX=clang++
clang: $(APP)
	scons


# FIN
