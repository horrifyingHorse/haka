haka:
	gcc ./src/haka.c ./src/hakaUtils.c ./src/hakaEventHandler.c -o haka.out -I./include/ -L/usr/local/lib -levdev

clean:
	rm -rf haka.out
	rm -rf prevFile.txt
