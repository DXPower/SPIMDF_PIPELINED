#pragma once

#include "Instruction.hpp"
#include <map>

namespace SPIMDF {
    class CPU {
        std::map<uint32_t, Instruction> program;
        std::map<uint32_t, int32_t> memory;
        std::array<int32_t, 32> registers { 5, 4, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                           , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        uint64_t cycle = 1;
        uint32_t pc;
        bool didJump = false;

        public:
        CPU(uint32_t pc = 0) : pc(pc) { };

        Instruction& Instr(uint32_t addr) { return program[addr]; };
        const Instruction& Instr(uint32_t addr) const { return program.at(addr); };

        Instruction& CurInstr() { return Instr(pc); };
        const Instruction& CurInstr() const { return Instr(pc); };

        int32_t& Mem(uint32_t addr) { return memory[addr]; };

        int32_t& Reg(uint8_t regAddr) { return registers[regAddr]; };
        const int32_t& Reg(uint8_t regAddr) const { return registers[regAddr]; };

        uint32_t GetPC() const { return pc; };
        uint64_t GetCycle() const { return cycle; };

        void Clock() {
            Instr(pc).Execute(*this);

            if (!didJump) {
                pc += 4;
            }

            cycle++;
            didJump = false;
        };

        // Sets PC = addr
        void Jump(uint32_t addr) {
            pc = addr;
            didJump = true;
        }
    };
}