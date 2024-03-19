CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = src/*.h src/main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o /home/scannerk/WebServer/$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf /home/scannerk/WebServer/$(OBJS) $(TARGET)