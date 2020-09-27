#pragma once

#include <string>

namespace SPIMDF {
    class Instruction;
    class CPU;

    #define DEF_EX(X) void X(CPU& cpu, const Instruction& instr);
    #define DEF_PR(X) std::string X(const Instruction& instr);

    namespace Executors {
        // Category 1
        DEF_EX(J);
        DEF_EX(JR);
        DEF_EX(BEQ);
        DEF_EX(BLTZ);
        DEF_EX(BGTZ);
        DEF_EX(SW); 
        DEF_EX(LW); 
        DEF_EX(SLL);
        DEF_EX(SRL);
        DEF_EX(SRA);
        DEF_EX(NOP);
        DEF_EX(BRK);
        // Category 2
        DEF_EX(ADD); 
        DEF_EX(SUB); 
        DEF_EX(MUL); 
        DEF_EX(AND); 
        DEF_EX(OR);  
        DEF_EX(XOR); 
        DEF_EX(NOR); 
        DEF_EX(SLT); 
        DEF_EX(ADDI);
        DEF_EX(ANDI);
        DEF_EX(ORI); 
        DEF_EX(XORI);
    }

    namespace Printers {
        // Category 1
        DEF_PR(J);
        DEF_PR(JR);
        DEF_PR(BEQ);
        DEF_PR(BLTZ);
        DEF_PR(BGTZ);
        DEF_PR(SW); 
        DEF_PR(LW); 
        DEF_PR(SLL);
        DEF_PR(SRL);
        DEF_PR(SRA);
        DEF_PR(NOP);
        DEF_PR(BRK);
        // Category 2
        DEF_PR(ADD); 
        DEF_PR(SUB); 
        DEF_PR(MUL); 
        DEF_PR(AND); 
        DEF_PR(OR);  
        DEF_PR(XOR); 
        DEF_PR(NOR); 
        DEF_PR(SLT); 
        DEF_PR(ADDI);
        DEF_PR(ANDI);
        DEF_PR(ORI); 
        DEF_PR(XORI);
    }

    #undef DEF_EX
    #undef DEF_PR
}