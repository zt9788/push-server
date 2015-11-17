#gcc -g server.c log.c md5.c util.c readconf.c thread_pool.c redis_pool.c -DDEBU
#G -D__EPOLL__ -o server -lpthread -L/usr/local/lib/ -lhiredis -luuid -lrt
CC=gcc
CFLAGS=-g#-Wall -g
LDFLAGS=-DDEBUG -D__EPOLL__ -lpthread -L/usr/local/lib/ -lhiredis -luuid -lrt 
all:clean server.o log.o md5.o messagestorage.o util.o readconf.o thread_pool.o redis_pool.o daemon.o leak_detector_c.o pushserver
.PHONY : all
#server.o:sc.h
#${CC}  ${CFLAGS}  -c server.c  ${LDFLAGS}
log.o:log.h
	${CC} ${CFLAGS} -c log.c ${LDFLAGS}
md5.o:md5.h
	${CC} ${CFLAGS} -c md5.c ${LDFLAGS}
util.o:util.h
	${CC} ${CFLAGS} -c util.c ${LDFLAGS}
readconf.o:readconf.h
	${CC} ${CFLAGS} -c readconf.c ${LDFLAGS}
thread_pool.o:thread_pool.h
	${CC} ${CFLAGS} -c thread_pool.c ${LDFLAGS}
redis_pool.o:redis_pool.h
	${CC} ${CFLAGS} -c redis_pool.c ${LDFLAGS}
leak_detector_c.o:leak_detector_c.h
	${CC} ${CFLAGS} -c leak_detector_c.c ${LDFLAGS}
messagestorage.o:messagestorage.h
	${CC} ${CFLAGS} -c messagestorage.c ${LDFLAGS}
server.o:sc.h
	${CC}  ${CFLAGS}  -c server.c  ${LDFLAGS}
daemon.o :daemon.h
	${CC} ${CFLAGS} -c daemon.c ${LDFLAGS}

pushserver:
	${CC} ${CFLAGS}  server.o messagestorage.o daemon.o log.o leak_detector_c.o md5.o util.o readconf.o thread_pool.o redis_pool.o -o pushserver ${LDFLAGS}

clean:  
	rm  -rf  *.o  
	rm  -rf  pushserver  
