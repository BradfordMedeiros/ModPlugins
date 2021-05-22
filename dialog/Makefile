
all: plugin binary

plugin: plugin.cpp
	@g++ -fPIC -shared plugin.cpp -o plugin.so

binary: plugin.cpp
	@g++ -D BINARY_MODE plugin.cpp -o plugin
clean: 
	rm -f plugin.so plugin.o plugin

