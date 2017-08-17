OBJS = util/GLog.o util/YamlConf.o util/SocketBuffer.o util/SocketConnection.o PushServer.o
CFLAGS = -W -Wall -Wunused-value -std=c++11 -g -rdynamic
DEPENDS = lib/glog/libglog.a lib/libuv/libuv.a lib/yaml/libyaml-cpp.a -Llib/openssl/lib -Llib/nghttp2/lib -Llib/c-ares/lib lib/curl/lib/libcurl.a -lssl -lnghttp2 -lcares -lcrypto -lz -lpthread
INCLUDE = -I. -Iutil/ -Ilib/ -Ilib/libuv/include -Ilib/curl/include -Ilib/json/include

bin/push_server: main.cpp main.h $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(INCLUDE) $(DEPENDS)

util/GLog.o: util/GLog.cpp util/GLog.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/YamlConf.o: util/YamlConf.cpp util/YamlConf.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/SocketBuffer.o: util/SocketBuffer.cpp util/SocketBuffer.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

util/SocketConnection.o: util/SocketConnection.cpp util/SocketConnection.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

PushServer.o: PushServer.cpp PushServer.h
	$(CXX) $(CFLAGS) -c $< $(INCLUDE) -o $@

.PHONY: clean start stop
clean:
	-$(MAKE) --no-print-directory stop
	-$(RM) util/*.o util/*.gch
	-$(RM) bin/push_server *.o *.gch
	-$(RM) -rf run/supervise
	-$(RM) log/*

start:
	-$(MAKE) --no-print-directory stop
	-$(MAKE) --no-print-directory
	cd run && bash run
#./bin/supervise.push_server run/ &

stop:
	-killall supervise.push_server 2>/dev/null
	-killall push_server 2>/dev/null
