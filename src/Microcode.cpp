#include "Microcode.hpp"
#include "CPU.hpp"
#include <cstdio>
#include "Instruction.hpp"
#include <sstream>

using namespace SPIMDF;

#define EX_FUNC(X) void Executors::X(CPU& cpu, const Instruction& in)
#define PR_FUNC(X) std::string Printers::X(const Instruction& in)

#define EX_NULL(X) EX_FUNC(X) { printf(#X " executor undefined"); }
#define PR_NULL(X) PR_FUNC(X) { return std::string(#X " printer undefined"); }

// Category 1

EX_FUNC(J) {
    const ISA::JType& format = in.GetFormat<ISA::JType>();
    cpu.Jump((cpu.GetPC() & 0xF0000000) | (format.index << 2));
}

EX_FUNC(JR) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Jump(cpu.Reg(format.rs));
}

EX_FUNC(BEQ) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();

    if (cpu.Reg(format.rs) == cpu.Reg(format.rt))
        cpu.Jump(cpu.GetPC() + (format.imm * 4));
}

EX_FUNC(BLTZ) {
    const ISA::IType& format = in.GetFormat<ISA::IType>();

    if (cpu.Reg(format.rs) < 0)
        cpu.Jump(cpu.GetPC() + (format.imm * 4));
}

EX_FUNC(BGTZ) {
    const auto& format = in.GetFormat<ISA::IType>();

    if (cpu.Reg(format.rs) > 0)
        cpu.Jump(cpu.GetPC() + (format.imm * 4));
}

EX_FUNC(SW) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Mem(cpu.Reg(format.rs) + format.imm) = cpu.Reg(format.rt);
}

EX_FUNC(LW) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Reg(format.rt) = cpu.Mem(cpu.Reg(format.rs) + format.imm);
}

EX_FUNC(SLL) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rt) << format.sa;
}

EX_FUNC(SRL) {
    const auto& format = in.GetFormat<ISA::RType>();
    // Cast to unsigned int to do guarantee logical shift, then shift by shamt
    cpu.Reg(format.rd) = static_cast<int32_t>(static_cast<uint32_t>(cpu.Reg(format.rt)) >> format.sa);
}

EX_FUNC(SRA) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rt) >> format.sa;
}

EX_FUNC(NOP) { }

EX_FUNC(BRK) { }

// Category 2
EX_FUNC(ADD) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) + cpu.Reg(format.rt);
}

EX_FUNC(SUB) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) - cpu.Reg(format.rt);
}

EX_FUNC(MUL) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) * cpu.Reg(format.rt);
}

EX_FUNC(AND) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) & cpu.Reg(format.rt);
}

EX_FUNC(OR) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) | cpu.Reg(format.rt);
} 

EX_FUNC(XOR) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) ^ cpu.Reg(format.rt);
}

EX_FUNC(NOR) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = ~(cpu.Reg(format.rs) | cpu.Reg(format.rt));
}

EX_FUNC(SLT) {
    const auto& format = in.GetFormat<ISA::RType>();
    cpu.Reg(format.rd) = cpu.Reg(format.rs) < cpu.Reg(format.rt);
}

EX_FUNC(ADDI) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Reg(format.rt) = cpu.Reg(format.rs) + format.imm;
}

EX_FUNC(ANDI) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Reg(format.rt) = cpu.Reg(format.rs) & static_cast<uint32_t>(format.imm); // Zero extend immediate
}

EX_FUNC(ORI) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Reg(format.rt) = cpu.Reg(format.rs) | static_cast<uint32_t>(format.imm); // Zero extend immediate
}

EX_FUNC(XORI) {
    const auto& format = in.GetFormat<ISA::IType>();
    cpu.Reg(format.rt) = cpu.Reg(format.rs) ^ static_cast<uint32_t>(format.imm); // Zero extend immediate
}

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

PR_FUNC(BRK) {
    return "BREAK";
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
