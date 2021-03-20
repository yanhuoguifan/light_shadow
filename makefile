light_shadow_compile:conf.o arg.o init.o thread.o light_shadow.o
	cp -r ./ ../alight

test:./source/test.cpp ./source/conf.cpp ./source/conf.h ./source/arg.cpp ./source/arg.h ./source/init.cpp ./source/init.h ./source/thread.cpp ./source/thread.h ./source/light_shadow.h ./source/light_shadow.cpp
	g++ -c -g ./source/test.cpp -o ./test_bin/test.o -O4
	g++ -c -g ./source/conf.cpp -o ./test_bin/conf.o -pthread -O4
	g++ -c -g ./source/arg.cpp -o ./test_bin/arg.o -O4  
	g++ -c -g ./source/init.cpp -o ./test_bin/init.o -O4 
	g++ -c -g ./source/thread.cpp -o ./test_bin/thread.o -pthread -O4
	g++ -c -g ./source/light_shadow.cpp -o ./test_bin/light_shadow.o -pthread -O4
	g++ -g ./test_bin/test.o ./test_bin/conf.o ./test_bin/arg.o ./test_bin/init.o ./test_bin/thread.o ./test_bin/light_shadow.o -o ./test_bin/test -O4 -pthread

conf.o:./source/conf.cpp ./source/conf.h
	g++ -c ./source/conf.cpp -o ./bin/conf.o -pthread -O4

arg.o:./source/arg.cpp ./source/arg.h
	g++ -c ./source/arg.cpp -o ./bin/arg.o -O4  

init.o:./source/init.cpp ./source/init.h
	g++ -c ./source/init.cpp -o ./bin/init.o -O4 

thread.o:./source/thread.cpp ./source/thread.h
	g++ -c ./source/thread.cpp -o ./bin/thread.o -pthread -O4

light_shadow.o:	./source/light_shadow.cpp ./source/light_shadow.h
	g++ -c  ./source/light_shadow.cpp -o ./bin/light_shadow.o -pthread -O4
