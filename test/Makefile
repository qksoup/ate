CXX = g++
PWD = $(shell pwd)
PROJ_ROOT = $(shell dirname $(PWD))
PROJ_LIB_DIR = $(PROJ_ROOT)/lib
INC_PATH = -I$(PROJ_ROOT)/include
SYSLIBS = -lpthread
LIBS = -late 
CXXFLAGS = -g -ansi -pedantic -W -Wall -D_REENTRANT $(INC_PATH)
#-Wshadow -Wwrite-strings 
SRCS = $(shell ls *.cpp)
OBJS = $(SRCS:.cpp=.o)


all: TestSocket.bin TestString.bin TestPool.bin TestThread.bin TestClient.bin TestServer.bin

TestString.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestString.o -o TestString.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

TestPool.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestPool.o -o TestPool.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

TestThread.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestThread.o -o TestThread.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

TestSocket.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestSocket.o -o TestSocket.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

TestClient.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestClient.o -o TestClient.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

TestServer.bin: $(OBJS)
	$(CXX) $(CXXFLAGS) TestServer.o -o TestServer.bin $(SYSLIBS) -L$(PROJ_LIB_DIR) $(LIBS)

%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@


clean:
	rm -f a.out *.o *~ core core.* *.bin
