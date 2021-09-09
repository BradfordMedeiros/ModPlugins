
all: build/dialog.so build/sql.so build/sequencer.so

clean:
	@(cd ./dialog && make clean)
	@(cd ./sql && make clean)
	@(cd ./sequencer && make clean)
	@(rm -r ./build)

build/dialog.so: ./dialog/plugin.so
	@mkdir -p ./build
	@cp ./dialog/plugin.so ./build/dialog.so

build/sql.so: ./sql/plugin.so
	@mkdir -p ./build
	@cp ./sql/plugin.so ./build/sql.so

build/sequencer.so: ./sequencer/plugin.so
	@mkdir -p ./build
	@cp ./sequencer/plugin.so ./build/sequencer.so

./dialog/plugin.so: ./dialog/Makefile
	@(cd dialog && make plugin)

./sql/plugin.so: ./sql/Makefile
	@(cd sql && make plugin)

./sequencer/plugin.so: ./sequencer/Makefile
	@(cd sequencer && make plugin)
