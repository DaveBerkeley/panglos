
.PHONY: all images

APP = tdd

ARGS:=

$(APP): 
	scons

clean:
	rm -r $(ODIR) -f $(APP) html latex
	scons -c
	find . -name "*~" | xargs rm -f

test: $(APP)
	./tdd $(ARGS)

doxygen: $(APP)
	doxygen

valgrind: $(APP)
	valgrind --leak-check=full ./$(APP) $(ARGS)

clang: export CC=clang
clang: export CXX=clang++
clang: $(APP)
	scons

images: docs/mutex.dot
	dot -T png $< -o images/mutex.png

ctags:
	ctags -R . 

# FIN
