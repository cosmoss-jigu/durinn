CXX=g++
CXXINC= -I$(LLVM9_HOME)/include -I$(LLVM9_HOME)/build/include
CXXFLAGS= -g -c -fPIC -std=c++17 $(CXXINC)

all: libciri.so librtciri.a

libciri.so: tracer_pass.o print_func_pass.o LoadStoreNumbering.o fence_src_map.o print_src_map.o print_lps_from_brs.o
	$(CXX) tracer_pass.o print_func_pass.o LoadStoreNumbering.o fence_src_map.o print_src_map.o print_lps_from_brs.o -shared -o libciri.so

tracer_pass.o: tracer_pass.cpp
	$(CXX) $(CXXFLAGS) tracer_pass.cpp -o tracer_pass.o

print_func_pass.o: print_func_pass.cpp
	$(CXX) $(CXXFLAGS) print_func_pass.cpp -o print_func_pass.o

LoadStoreNumbering.o: LoadStoreNumbering.cpp
	$(CXX) $(CXXFLAGS) LoadStoreNumbering.cpp -o LoadStoreNumbering.o

fence_src_map.o: fence_src_map.cpp
	$(CXX) $(CXXFLAGS) fence_src_map.cpp -o fence_src_map.o

print_src_map.o: print_src_map.cpp
	$(CXX) $(CXXFLAGS) print_src_map.cpp -o print_src_map.o

print_lps_from_brs.o: print_lps_from_brs.cpp
	$(CXX) $(CXXFLAGS) print_lps_from_brs.cpp -o print_lps_from_brs.o

librtciri.a: Tracing.o
	ar cru librtciri.a Tracing.o
	ranlib librtciri.a

Tracing.o: Tracing.cpp
	$(CXX) $(CXXFLAGS) Tracing.cpp -o Tracing.o

clean:
	rm -rf *.o *.a *.so
