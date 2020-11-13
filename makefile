FLAGS = -std=c++20 -D_SILENCE_CLANG_CONCEPTS_MESSAGE -Wno-format

all:
	compiledb make all -n

	C:\\Program Files\\LLVM\\bin\\clang++.exe ${FLAGS} -g -Isrc/ src/main.cpp src/Microcode.cpp src/Disassembler.cpp src/Execs.cpp -o MIPSsim.exe 
