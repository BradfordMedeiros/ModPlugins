
all: plugin binary

plugin: plugin.cpp
	@g++ -fPIC -shared -I /usr/include/guile/2.2/ plugin.cpp -lguile-2.2  -o plugin.so

binary: plugin.cpp
	@g++ -D BINARY_MODE -ggdb -I /usr/include/guile/2.2/ plugin.cpp -lguile-2.2 -o plugin
clean: 
	rm -f plugin.so plugin.o plugin

