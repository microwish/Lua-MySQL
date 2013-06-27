NAME = lua-mysql
VERSION = 0.1
DIST := $(NAME)-$(VERSION)

CC = gcc
RM = rm -rf

CFLAGS = -Wall -g -fPIC -I/home/microwish/lua/include -I/home/microwish/lib/mysql-client/include
#CFLAGS = -Wall -g -O2 -fPIC -I/home/microwish/lua/include -I/home/microwish/lib/mysql-client/include/mysql
LFLAGS = -shared -Wl,--rpath=/home/microwish/lib/mysql-client/lib/mysql -Wl,-Bstatic -L/home/microwish/lua/lib -llua -Wl,-Bdynamic -L/home/work/lib/mysql-client/lib/mysql -lmysqlclient
INSTALL_PATH = /home/microwish/lua-mysql/lib

all: mysql.so

mysql.so: mysql.o
  $(CC) -o $@ $< $(LFLAGS)

mysql.o: lmysqllib.c
	$(CC) -o $@ $(CFLAGS) -c $<

install: mysql.so
	install -D -s $< $(INSTALL_PATH)/$<

clean:
	$(RM) *.so *.o

dist:
	if [ -d $(DIST) ]; then $(RM) $(DIST); fi
	mkdir -p $(DIST)
	cp *.c Makefile $(DIST)/
	tar czvf $(DIST).tar.gz $(DIST)
	$(RM) $(DIST)

.PHONY: all clean dist
