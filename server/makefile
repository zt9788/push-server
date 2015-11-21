#gcc -g server.c log.c md5.c util.c readconf.c thread_pool.c redis_pool.c -DDEBU
#G -D__EPOLL__ -o server -lpthread -L/usr/local/lib/ -lhiredis -luuid -lrt
CC=gcc
CFLAGS=-g#-Wall -g
LDFLAGS=-DDEBUG -D__EPOLL__ -lpthread -L/usr/local/lib/ -lhiredis -luuid -lrt 
all:clean server.o log.o list.o list_iterator.o list_node.o md5.o extended_storage.o messagestorage.o util.o readconf.o thread_pool.o socket_plush.o redis_pool.o daemon.o parse_command.o leak_detector_c.o pushserver
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
parse_command.o:parse_command.h
	${CC} ${CFLAGS} -c parse_command.c ${LDFLAGS}
socket_plush.o:socket_plush.h
	${CC} ${CFLAGS} -c socket_plush.c ${LDFLAGS}
	
list.o:list.h
	${CC} -O3 -std=c99 -Wall -Wextra -Ideps -c list.c
list_iterator.o:list.h
	${CC} -O3 -std=c99 -Wall -Wextra -Ideps -c list_iterator.c
list_node.o:list.h
	${CC} -O3 -std=c99 -Wall -Wextra -Ideps -c list_node.c

extended_storage.o:extended_storage.h
	${CC} ${CFLAGS} -c extended_storage.c ${LDFLAGS}	
	
pushserver:
	${CC} ${CFLAGS}  server.o messagestorage.o extended_storage.o list.o list_iterator.o list_node.o daemon.o log.o leak_detector_c.o md5.o util.o socket_plush.o readconf.o thread_pool.o redis_pool.o parse_command.o -o pushserver ${LDFLAGS}

clean:  
	rm  -rf  *.o  
	rm  -rf  pushserver  
