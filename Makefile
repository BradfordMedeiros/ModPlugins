
all: build/dialog.so build/sql.so

clean:
	@(cd ./dialog && make clean)
	@(cd ./sql && make clean)

build/dialog.so: ./dialog/plugin.so
	@mkdir -p ./build
	@cp ./dialog/plugin.so ./build/dialog.so

build/sql.so: ./sql/plugin.so
	@mkdir -p ./build
	@cp ./sql/plugin.so ./build/sql.so

./dialog/plugin.so: ./dialog/Makefile
	@(cd dialog && make plugin)

./sql/plugin.so: ./sql/Makefile
	@(cd sql && make plugin)
