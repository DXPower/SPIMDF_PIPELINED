FLAGS = -std=c++20 -D_SILENCE_CLANG_CONCEPTS_MESSAGE -Wno-format

all:
	compiledb make all -n

	clang++ ${FLAGS} -g src/main.cpp src/Microcode.cpp src/Disassembler.cpp -o MIPSsim.exe 
