#include "Microcode.hpp"
#include "Instruction.hpp"
#include <cstdio>
#include <sstream>

using namespace SPIMDF;

#define EX_FUNC(X) void Executors::X(const Instruction& in)
#define PR_FUNC(X) std::string Printers::X(const Instruction& in)

#define EX_NULL(X) EX_FUNC(X) { printf(#X " executor undefined"); }
#define PR_NULL(X) PR_FUNC(X) { return std::string(#X " printer undefined"); }

EX_FUNC(J) {
    printf("Execute jump!\n");
}

// Category 1
EX_NULL(JR); 
EX_NULL(BEQ);
EX_NULL(BLTZ);
EX_NULL(BGTZ);
EX_NULL(SW); 
EX_NULL(LW); 
EX_NULL(SLL);
EX_NULL(SRL);
EX_NULL(SRA);
EX_NULL(NOP);
EX_NULL(BRK);
// Category 2
EX_NULL(ADD); 
EX_NULL(SUB); 
EX_NULL(MUL); 
EX_NULL(AND); 
EX_NULL(OR);  
EX_NULL(XOR); 
EX_NULL(NOR); 
EX_NULL(SLT); 
EX_NULL(ADDI);
EX_NULL(ANDI);
EX_NULL(ORI); 
EX_NULL(XORI);

std::string String_RType(const char* opcode, const ISA::RType& format) {
    std::ostringstream ss;

    ss << opcode
       << " R" << +format.rd
       << ", R" << +format.rs
       << ", R" << +format.rt;

    return ss.str();
}

std::string String_IType(const char* opcode, const ISA::IType& format, int mult = 1) {
    std::ostringstream ss;

    ss << opcode
       << " R" << +format.rt
       << ", R" << +format.rs
       << ", #" << (format.imm * mult);

    return ss.str();
}

PR_FUNC(J) {
    const ISA::JType& format = in.GetFormat<ISA::JType>();

    std::stringstream ss;
    ss << "J #" << (format.index << 2);

    return ss.str();
}

// Category 1
PR_FUNC(JR) {
    const ISA::RType& format = in.GetFormat<ISA::RType>();
    
    std::stringstream ss;
    ss << "JR R" << +format.rs;

    return ss.str();
}

PR_FUNC(BEQ) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();

    std::stringstream ss;
    ss << "BEQ R" << + format.rs << ", R" << +format.rt << ", #" << (format.imm * 4);

    return ss.str();
}

PR_FUNC(BLTZ) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();
    
    std::stringstream ss;
    ss << "BLTZ R" << +format.rs << ", #" << (format.imm * 4);

    return ss.str();
}

PR_FUNC(BGTZ) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();
    
    std::stringstream ss;
    ss << "BGTZ R" << +format.rs << ", #" << (format.imm * 4);

    return ss.str();
}

PR_FUNC(SW) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();

    std::stringstream ss;
    ss << "SW R" << +format.rt << ", " << format.imm << "(R" << +format.rs << ")";

    return ss.str();
}

// 010111 10000 00011 0000000101010100
// opcode base  rt    imm

PR_FUNC(LW) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();

    std::stringstream ss;
    ss << "LW R" << +format.rt << ", " << format.imm << "(R" << +format.rs  << ")";

    return ss.str();
}

PR_FUNC(SLL) {
    const ISA::RType& format = in.GetFormat<ISA::RType>();

    std::stringstream ss;
    ss << "SLL R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

    return ss.str();
}

PR_FUNC(SRL) {
    const ISA::RType& format = in.GetFormat<ISA::RType>();

    std::stringstream ss;
    ss << "SRL R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

    return ss.str();
}

PR_FUNC(SRA) {
    const ISA::RType& format = in.GetFormat<ISA::RType>();

    std::stringstream ss;
    ss << "SRA R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

    return ss.str();
}

PR_FUNC(NOP) {
    return "NOP";
}

// Category 2
#define PR_CAT2_RTYPE(X) PR_FUNC(X) { \
    const ISA::RType& format = in.GetFormat<ISA::RType>(); \
    return String_RType(#X, format); \
}

#define PR_CAT2_ITYPE(X) PR_FUNC(X) { \
    const ISA::IType& format = in.GetFormat<ISA::IType>(); \
    return String_IType(#X, format); \
}

PR_CAT2_RTYPE(ADD)
PR_CAT2_RTYPE(SUB); 
PR_CAT2_RTYPE(MUL); 
PR_CAT2_RTYPE(AND); 
PR_CAT2_RTYPE(OR);  
PR_CAT2_RTYPE(XOR); 
PR_CAT2_RTYPE(NOR); 
PR_CAT2_RTYPE(SLT); 
PR_CAT2_ITYPE(ADDI);
PR_CAT2_ITYPE(ANDI);
PR_CAT2_ITYPE(ORI); 
PR_CAT2_ITYPE(XORI);
