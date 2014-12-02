cdef extern from "../src/main.cpp":
     int main(int argc, char** argv)

def run():
    main(0, <char**>0)

