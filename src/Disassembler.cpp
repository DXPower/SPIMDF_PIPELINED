#include "Disassembler.hpp"
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
const char* BRK_OPCD = "010101";

std::unordered_map<std::string, std::function<DecodeFunc>> opcodeDecoders {
    // Category 1
      { "010000", DecodeHelper<ISA::J   , ISA::detail::Decode_J   > }
    , { "010001", DecodeHelper<ISA::JR  , ISA::detail::Decode_JR  > }
    , { "010010", DecodeHelper<ISA::BEQ , ISA::detail::Decode_BEQ > }
    , { "010011", DecodeHelper<ISA::BLTZ, ISA::detail::Decode_BLTZ> }
    , { "010100", DecodeHelper<ISA::BGTZ, ISA::detail::Decode_BGTZ> }
    , { BRK_OPCD, DecodeHelper<ISA::NOP , ISA::detail::Decode_NOP > } // BREAK uses NOP as a placeholder
    , { "010110", DecodeHelper<ISA::SW  , ISA::detail::Decode_SW  > }
    , { "010111", DecodeHelper<ISA::LW  , ISA::detail::Decode_LW  > }
    , { "011000", DecodeHelper<ISA::SLL , ISA::detail::Decode_SLL > }
    , { "011001", DecodeHelper<ISA::SRL , ISA::detail::Decode_SRL > }
    , { "011010", DecodeHelper<ISA::SRA , ISA::detail::Decode_SRA > }
    , { "011011", DecodeHelper<ISA::NOP , ISA::detail::Decode_NOP > }

    // Category 2
    , { "110000", DecodeHelper<ISA::ADD , ISA::detail::Decode_ADD > }
    , { "110001", DecodeHelper<ISA::SUB , ISA::detail::Decode_SUB > }
    , { "110010", DecodeHelper<ISA::MUL , ISA::detail::Decode_MUL > }
    , { "110011", DecodeHelper<ISA::AND , ISA::detail::Decode_AND > }
    , { "110100", DecodeHelper<ISA::OR  , ISA::detail::Decode_OR  > }
    , { "110101", DecodeHelper<ISA::XOR , ISA::detail::Decode_XOR > }
    , { "110110", DecodeHelper<ISA::NOR , ISA::detail::Decode_NOR > }
    , { "110111", DecodeHelper<ISA::SLT , ISA::detail::Decode_SLT > }
    , { "111000", DecodeHelper<ISA::ADDI, ISA::detail::Decode_ADDI> }
    , { "111001", DecodeHelper<ISA::ANDI, ISA::detail::Decode_ANDI> }
    , { "111010", DecodeHelper<ISA::ORI , ISA::detail::Decode_ORI > }
    , { "111011", DecodeHelper<ISA::XORI, ISA::detail::Decode_XORI> }
};

Instruction DecodeMachineCode(const std::string& mach) {
    std::string opcode = mach.substr(0, 6);

    if (opcode != BRK_OPCD) {
        auto decoder = opcodeDecoders[opcode];
        Instruction instr = decoder(mach);

        return instr;
    } else {
        printf("Break!\n");
        std::terminate();
    }
}

void SPIMDF::Disassemble(const char* filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        printf("File not found\n");
        std::terminate();
    }

    std::string machCode;
    uint32_t curAddr = 256;

    while (file >> machCode) {
        std::string category = machCode.substr(0, 2);

        Instruction instr = DecodeMachineCode(machCode);

        printf("%s\t%u\t%s\n", machCode.c_str(), curAddr, instr.ToString().c_str());

        curAddr++;
    }
}