#include "Disassembler.hpp"
#include "CPU.hpp"
#include <fstream>
#include "Instruction.hpp"
#include "ISA.hpp"
#include <unordered_map>

using namespace SPIMDF;

template<auto Factory, auto Decoder>
Instruction DecodeHelper(const std::string& mach) {
    auto format = Decoder(mach);
    return Instruction::CreateFromFormat<Factory>(format);
}

using DecodeFunc = Instruction(const std::string&);

std::unordered_map<std::string, std::pair<ISA::Opcode, std::function<DecodeFunc>>> opcodeDecoders {
    // Category 1
      { "010000", { ISA::Opcode::J   , DecodeHelper<ISA::J   , ISA::detail::Decode_J   > } } 
    , { "010001", { ISA::Opcode::JR  , DecodeHelper<ISA::JR  , ISA::detail::Decode_JR  > } } 
    , { "010010", { ISA::Opcode::BEQ , DecodeHelper<ISA::BEQ , ISA::detail::Decode_BEQ > } } 
    , { "010011", { ISA::Opcode::BLTZ, DecodeHelper<ISA::BLTZ, ISA::detail::Decode_BLTZ> } } 
    , { "010100", { ISA::Opcode::BGTZ, DecodeHelper<ISA::BGTZ, ISA::detail::Decode_BGTZ> } } 
    , { "010101", { ISA::Opcode::BRK , DecodeHelper<ISA::BRK , ISA::detail::Decode_BRK > } }  
    , { "010110", { ISA::Opcode::SW  , DecodeHelper<ISA::SW  , ISA::detail::Decode_SW  > } } 
    , { "010111", { ISA::Opcode::LW  , DecodeHelper<ISA::LW  , ISA::detail::Decode_LW  > } } 
    , { "011000", { ISA::Opcode::SLL , DecodeHelper<ISA::SLL , ISA::detail::Decode_SLL > } } 
    , { "011001", { ISA::Opcode::SRL , DecodeHelper<ISA::SRL , ISA::detail::Decode_SRL > } } 
    , { "011010", { ISA::Opcode::SRA , DecodeHelper<ISA::SRA , ISA::detail::Decode_SRA > } } 
    , { "011011", { ISA::Opcode::NOP , DecodeHelper<ISA::NOP , ISA::detail::Decode_NOP > } } 

    // Category 2
    , { "110000", { ISA::Opcode::ADD , DecodeHelper<ISA::ADD , ISA::detail::Decode_ADD > } }
    , { "110001", { ISA::Opcode::SUB , DecodeHelper<ISA::SUB , ISA::detail::Decode_SUB > } }
    , { "110010", { ISA::Opcode::MUL , DecodeHelper<ISA::MUL , ISA::detail::Decode_MUL > } }
    , { "110011", { ISA::Opcode::AND , DecodeHelper<ISA::AND , ISA::detail::Decode_AND > } }
    , { "110100", { ISA::Opcode::OR  , DecodeHelper<ISA::OR  , ISA::detail::Decode_OR  > } }
    , { "110101", { ISA::Opcode::XOR , DecodeHelper<ISA::XOR , ISA::detail::Decode_XOR > } }
    , { "110110", { ISA::Opcode::NOR , DecodeHelper<ISA::NOR , ISA::detail::Decode_NOR > } }
    , { "110111", { ISA::Opcode::SLT , DecodeHelper<ISA::SLT , ISA::detail::Decode_SLT > } }
    , { "111000", { ISA::Opcode::ADDI, DecodeHelper<ISA::ADDI, ISA::detail::Decode_ADDI> } }
    , { "111001", { ISA::Opcode::ANDI, DecodeHelper<ISA::ANDI, ISA::detail::Decode_ANDI> } }
    , { "111010", { ISA::Opcode::ORI , DecodeHelper<ISA::ORI , ISA::detail::Decode_ORI > } }
    , { "111011", { ISA::Opcode::XORI, DecodeHelper<ISA::XORI, ISA::detail::Decode_XORI> } }
};

Instruction DecodeMachineCode(const std::string& mach) {
    std::string binOpcode = mach.substr(0, 6);

    const auto& [opcode, decoder] = opcodeDecoders[binOpcode];

    return decoder(mach);
}

int32_t DecodeProgramDatum(const std::string& mach) {
    return FromTwosComp(mach);
}

void SPIMDF::Disassemble(const char* filename, CPU& cpu) {
    std::ifstream file(filename);
    std::ofstream output("disassembly.txt", std::ios::binary);
    char buffer[200];

    if (!file.is_open()) {
        output << "File not found" << std::endl;
        std::terminate();
    }

    std::string machCode;
    uint32_t curAddr = 256;

    while (file >> machCode) {
        std::string category = machCode.substr(0, 2);

        Instruction instr = DecodeMachineCode(machCode);

        cpu.Instr(curAddr) = instr;
       
        sprintf(buffer, "%s\t%u\t%s\n", machCode.c_str(), curAddr, instr.ToString().c_str());
        output << buffer;

        curAddr += 4;
        
        if (instr.opcode == ISA::Opcode::BRK)
            break;
    }

    while (file >> machCode) {
        int32_t datum = DecodeProgramDatum(machCode);

        cpu.Mem(curAddr) = datum;

        sprintf(buffer, "%s\t%u\t%i\n", machCode.c_str(), curAddr, datum);
        output << buffer;

        curAddr += 4;
    }

    output << std::flush;
    output.close();
}