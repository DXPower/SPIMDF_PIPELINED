#pragma once

#include "Instruction.hpp"
#include "Buffer.hpp"
#include "Execs.hpp"
#include <map>

namespace SPIMDF {
    class CPU {
        std::map<uint32_t, Instruction> program;
        std::map<uint32_t, int32_t> memory;
        std::array<int32_t, 32> registers { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                          , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        uint64_t cycle = 1;
        uint32_t pc;

        public:
        // Queues
        struct {
            PreIssueQueue preIssue;
            PreALUQueue preALU;
            PostALUQueue postALU;
            PreMemALUQueue preMemALU;
            PreMemQueue preMem;
            PostMemQueue postMem;
        } queues;

        // Executors
        struct {
            FetchExec fetch;
            IssueExec issue;
            ALUExec alu;
            MemALUExec memALU;
            MemExec mem;
        } executors;

        CPU(uint32_t pc = 0)
        : pc(pc)
        , executors({
              FetchExec(*this)
            , IssueExec(*this)
            , ALUExec(*this)
            , MemALUExec(*this)
            , MemExec(*this) 
        })
        { };

        Instruction& Instr(uint32_t addr) { return program[addr]; };
        const Instruction& Instr(uint32_t addr) const { return program.at(addr); };

        Instruction& CurInstr() { return Instr(pc); };
        const Instruction& CurInstr() const { return Instr(pc); };

        int32_t& Mem(uint32_t addr) { return memory[addr]; };
        const auto& GetAllMem() const { return memory; };

        int32_t& Reg(uint8_t regAddr) { return registers[regAddr]; };
        const int32_t& Reg(uint8_t regAddr) const { return registers[regAddr]; };

        uint32_t GetPC() const { return pc; };
        uint64_t GetCycle() const { return cycle; };

        void Clock() {
            // Point PC to delay slot to support correct branching operation
            // pc += 4; // PC now points to delay slot
            // Instr(pc - 4).Execute(*this); // But execute the current instruction

            executors.fetch.Consume();
            executors.fetch.Produce();

            cycle++;
        };

        // Sets PC = addr
        void Jump(uint32_t addr) {
            pc = addr;
        }

        // Sets PC = PC + offset
        void RelJump(int32_t offset) {
            pc = pc + offset;
        }
    };
}