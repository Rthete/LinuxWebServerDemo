CC = g++
CFLAG = -Wall -c
SRCDIR = src
BUILDDIR = build

OBJS = $(BUILDDIR)/main.o \
	   $(BUILDDIR)/httpserver.o $(BUILDDIR)/httpresponse.o \
	   $(BUILDDIR)/httpcontent.o $(BUILDDIR)/httprequest.o \
	   $(BUILDDIR)/tcpserver.o \
       $(BUILDDIR)/acceptor.o $(BUILDDIR)/channel.o $(BUILDDIR)/eventloop.o \
       $(BUILDDIR)/epoller.o $(BUILDDIR)/tcpconnection.o $(BUILDDIR)/thread.o \
	   $(BUILDDIR)/eventloopthreadpool.o $(BUILDDIR)/eventloopthread.o \
	   $(BUILDDIR)/buffer.o \
	   $(TFLAG)

TFLAG = -lpthread
NDEBUG = -DNDEBUG
BUILD = -O3 -DNDEBUG

$(BUILDDIR)/web_server: $(OBJS)
	$(CC) $^ -o $@ $(BUILD)

$(BUILDDIR)/main.o: $(SRCDIR)/main.cc $(SRCDIR)/eventloop.h $(SRCDIR)/address.h \
					$(SRCDIR)/httpserver.h $(SRCDIR)/httprequest.h $(SRCDIR)/httpresponse.h \
					$(SRCDIR)/httpresponsefile.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/httpserver.o: $(SRCDIR)/httpserver.cc $(SRCDIR)/httpserver.h $(SRCDIR)/buffer.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/httpresponse.o: $(SRCDIR)/httpresponse.cc $(SRCDIR)/httpresponse.h $(SRCDIR)/buffer.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/httpcontent.o: $(SRCDIR)/httpcontent.cc $(SRCDIR)/httpcontent.h $(SRCDIR)/httprequest.h \
						   $(SRCDIR)/httpstate.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/httprequest.o: $(SRCDIR)/httprequest.cc $(SRCDIR)/httprequest.h $(SRCDIR)/httpstate.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/tcpserver.o: $(SRCDIR)/tcpserver.cc $(SRCDIR)/tcpserver.h $(SRCDIR)/acceptor.h $(SRCDIR)/tcpconnection.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/acceptor.o: $(SRCDIR)/acceptor.cc $(SRCDIR)/acceptor.h $(SRCDIR)/address.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/channel.o: $(SRCDIR)/channel.cc $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/eventloop.o: $(SRCDIR)/eventloop.cc $(SRCDIR)/eventloop.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/epoller.o: $(SRCDIR)/epoller.cc $(SRCDIR)/epoller.h $(SRCDIR)/channel.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/tcpconnection.o: $(SRCDIR)/tcpconnection.cc $(SRCDIR)/tcpconnection.h $(SRCDIR)/channel.h \
								$(SRCDIR)/buffer.h $(SRCDIR)/httpcontent.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/eventloopthreadpool.o: $(SRCDIR)/eventloopthreadpool.cc $(SRCDIR)/eventloopthreadpool.h $(SRCDIR)/eventloopthread.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/eventloopthread.o: $(SRCDIR)/eventloopthread.cc $(SRCDIR)/eventloopthread.h $(SRCDIR)/mutex.h \
							   $(SRCDIR)/condition.h  $(SRCDIR)/eventloop.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/thread.o: $(SRCDIR)/thread.cc $(SRCDIR)/thread.h $(SRCDIR)/latch.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

$(BUILDDIR)/buffer.o: $(SRCDIR)/buffer.cc $(SRCDIR)/buffer.h
	$(CC) $(CFLAG) $< -o $@ $(BUILD)

debug: $(OBJS)
	$(CC) $^ -o $(BUILDDIR)/web_server

release: $(BUILDDIR)/web_server

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/*.o
