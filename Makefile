haka:
	gcc ./src/haka.c -o haka.out -I./include/ -L/usr/local/lib -levdev

clean:
	rm -rf haka.out
