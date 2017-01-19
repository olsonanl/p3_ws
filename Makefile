
ifeq ($(wildcard /Library),) 

BUILD_TOOLS = /disks/patric-common/runtime/gcc-4.9.3

#CXX = /opt/rh/devtoolset-2/root/usr/bin/g++
CXX = $(BUILD_TOOLS)/bin/g++
BOOST = $(BUILD_TOOLS)
#BOOST = /scratch/olson/boost
STDCPP = -std=c++14 
THREADLIB = -lpthread -lrt

CXX_LDFLAGS = -Wl,-rpath,$(BUILD_TOOLS)/lib64

else 

BOOST = /Users/olson/P3/p3ws/install
STDCPP = -stdlib=libc++ -std=gnu++14  

endif

default: p3ws

BUILD_DEBUG = 1

ifeq ($(BUILD_DEBUG),1)
OPT = -g

# OPT = -g -DBOOST_ASIO_ENABLE_HANDLER_TRACKING

else
ifeq ($(BUILD_DEBUG),2)
OPT = -g -O3

# OPT = -g -DBOOST_ASIO_ENABLE_HANDLER_TRACKING

else

OPT = -O3
#OPT = -O2 -pg
#OPT = -g -O3

endif
endif

MONGOCXX_CFLAGS = -I$(BOOST)/include/mongocxx/v_noabi -I$(BOOST)/include/bsoncxx/v_noabi
MONGOCXX_LIBS = -L$(BOOST)/lib -lmongocxx -lbsoncxx

PROFILER_DIR = /scratch/olson/gperftools

#PROFILER_LIB = -L$(PROFILER_DIR)/lib -lprofiler
#PROFILER_INC = -DGPROFILER -I$(PROFILER_DIR)/include

INC = -I$(BOOST)/include 

USE_TBB = 0
USE_NUMA = 0

ifneq ($(USE_TBB),0)

TBB_DEBUG = 0
TBB_INC_DIR = $(BUILD_TOOLS)/include

ifeq ($(TBB_DEBUG),1)
TBB_LIB_DIR = $(BUILD_TOOLS)/lib
TBB_LIBS = -ltbbmalloc_debug -ltbb_debug
else
TBB_LIB_DIR = $(BUILD_TOOLS)/lib
TBB_LIBS = -ltbbmalloc -ltbb
endif

ifneq ($(TBB_INC_DIR),"")
TBB_CFLAGS = -DUSE_TBB -I$(TBB_INC_DIR)
TBB_LIB = -L$(TBB_LIB_DIR) $(TBB_LIBS)
TBB_LDFLAGS = -Wl,-rpath,$(TBB_LIB_DIR) 
endif

endif

ifneq ($(USE_NUMA),0)

NUMA_LIBS = -lhwloc
NUMA_CFLAGS = -DUSE_NUMA

endif

#BLCR_DIR = /disks/patric-common/blcr

ifneq ($(BLCR_DIR),)
BLCR_CFLAGS = -DBLCR_SUPPORT -I$(BLCR_DIR)/include
BLCR_LIB = -L$(BLCR_DIR)/lib -lcr
endif

JSON_CFLAGS = -I../json/src

CXXFLAGS = $(STDCPP) $(INC) $(OPT) $(PROFILER_INC) $(BLCR_CFLAGS) $(TBB_CFLAGS) $(NUMA_CFLAGS) $(JSON_CFLAGS) $(MONGOCXX_CFLAGS)
CFLAGS = $(INC) $(OPT) $(PROFILER_INC) 

# LDFLAGS  = -static
LDFLAGS = -Wl,-rpath -Wl,$(BOOST)/lib $(TBB_LDFLAGS) $(CXX_LDFLAGS)

LIBS = $(BOOST)/lib/libboost_system.a \
	$(BOOST)/lib/libboost_filesystem.a \
	$(BOOST)/lib/libboost_timer.a \
	$(BOOST)/lib/libboost_chrono.a \
	$(BOOST)/lib/libboost_iostreams.a \
	$(BOOST)/lib/libboost_regex.a \
	$(BOOST)/lib/libboost_thread.a \
	$(BOOST)/lib/libboost_program_options.a \
	$(MONGOCXX_LIBS) \
	$(THREADLIB) \
	$(PROFILER_LIB) \
	$(BLCR_LIB) \
	$(TBB_LIB) \
	$(NUMA_LIBS)

p3ws: p3ws.o kserver.o krequest.o jsonrpc_handler.o ws.o ws_client.o auth_token.o
	$(CXX) $(LDFLAGS) $(OPT) -o $@ $^ $(LIBS)

auth_token:  auth_token.o
	$(CXX) $(LDFLAGS) $(OPT) -o $@ $^ $(LIBS)

curl_aio.o: curl_aio.h
curl_aio: curl_aio.o
	$(CXX) $(LDFLAGS) $(OPT) -o $@ $^ $(LIBS) -lcurl

daytime: daytime.o
	$(CXX) $(LDFLAGS) $(OPT) -o $@ $^ $(LIBS) -lcurl

clean:
	rm -f *.o p3ws

depend:
	makedepend -Y *.cc

# DO NOT DELETE

jsonrpc_handler.o: jsonrpc_handler.h kserver.h krequest.h
krequest.o: krequest.h global.h debug.h kserver.h
kserver.o: kserver.h krequest.h global.h
p3ws.o: kserver.h krequest.h jsonrpc_handler.h ws.h ws_client.h global.h
ws.o: ws.h ws_client.h
ws_client.o: ws_client.h ws.h
