CXX = g++
PWD = $(shell pwd)
PROJ_ROOT = $(shell dirname $(PWD))
PROJ_LIB_DIR = $(PROJ_ROOT)/lib
INC_PATH = -I$(PROJ_ROOT)/include
LIBS = 
CXXFLAGS = -g -ansi -pedantic -W -Wall -D_REENTRANT $(INC_PATH)
#-Wshadow -Wwrite-strings
SRCS = $(shell ls *.cpp)
OBJS = $(SRCS:.cpp=.o)

DEPDIR = .deps
df = $(*F)

all: build_static_lib

build_static_lib: mkdepdir $(OBJS) 
	ar rcs $(PROJ_LIB_DIR)/libate.a $(OBJS)

mkdepdir:
	mkdir -p $(DEPDIR)
	mkdir -p $(PROJ_LIB_DIR)

%.o : %.cpp 
	$(CXX) -c $(CXXFLAGS) -MMD $< -o $@

	@mv -f $*.d $(DEPDIR) 
	@sed -e 's|.*:|$*.o:|' < $(DEPDIR)/$*.d > $(DEPDIR)/$*.P 
	@sed -e 's/.*://' -e 's/\\$$//' < $(DEPDIR)/$*.d | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> $(DEPDIR)/$*.P 
	@rm -f $(DEPDIR)/$*.d


clean:
	rm -rf $(DEPDIR) *.d a.out *.o $(PROJ_LIB_DIR)/*.a $(PROJ_LIB_DIR)/*.so *~ core core.*

-include $(SRCS:%.cpp=$(DEPDIR)/%.P)

