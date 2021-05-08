
all: plugin

plugin: plugin.cpp
	@g++ -fPIC -shared plugin.cpp -o plugin.so

clean: 
	rm -f plugin.so plugin.o

