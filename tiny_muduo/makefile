CC = g++
CFLAG = -Wall -c
SRCDIR = src
BUILDDIR = build

OBJS = $(BUILDDIR)/main.o $(BUILDDIR)/echo.o $(BUILDDIR)/tcpserver.o \
       $(BUILDDIR)/acceptor.o $(BUILDDIR)/channel.o $(BUILDDIR)/eventloop.o \
       $(BUILDDIR)/epoller.o $(BUILDDIR)/tcpconnectionptr.o

web_server: $(OBJS)
	$(CC) $^ -o $@

$(BUILDDIR)/main.o: $(SRCDIR)/main.cc $(SRCDIR)/eventloop.h $(SRCDIR)/address.h $(SRCDIR)/echo.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/echo.o: $(SRCDIR)/echo.cc $(SRCDIR)/echo.h $(SRCDIR)/tcpserver.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/tcpserver.o: $(SRCDIR)/tcpserver.cc $(SRCDIR)/tcpserver.h $(SRCDIR)/acceptor.h $(SRCDIR)/tcpconnectionptr.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/acceptor.o: $(SRCDIR)/acceptor.cc $(SRCDIR)/acceptor.h $(SRCDIR)/address.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/channel.o: $(SRCDIR)/channel.cc $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/eventloop.o: $(SRCDIR)/eventloop.cc $(SRCDIR)/eventloop.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/epoller.o: $(SRCDIR)/epoller.cc $(SRCDIR)/epoller.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@

$(BUILDDIR)/tcpconnectionptr.o: $(SRCDIR)/tcpconnectionptr.cc $(SRCDIR)/tcpconnectionptr.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/*.o