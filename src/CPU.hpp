#pragma once

#include "Instruction.hpp"
#include "Buffer.hpp"
#include "Execs.hpp"
#include <map>

namespace SPIMDF {
    class CPU {
        struct Register_t {
            int32_t value = 0;
            bool pendingRead = false;
            bool pendingWrite = false;
        };

        std::map<uint32_t, Instruction> program;
        std::map<uint32_t, int32_t> memory;
        std::array<Register_t, 32> registers;

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

        int32_t& Reg(uint8_t regAddr) { return registers[regAddr].value; };
        const int32_t& Reg(uint8_t regAddr) const { return registers[regAddr].value; };

        bool IsRegPendingRead(uint8_t regAddr) const { return registers[regAddr].pendingRead; };
        bool IsRegPendingWrite(uint8_t regAddr) const { return registers[regAddr].pendingWrite; };
        void SetRegPendingRead(uint8_t regAddr, bool flag) { registers[regAddr].pendingRead = flag; };
        void SetRegPendingWrite(uint8_t regAddr, bool flag) { registers[regAddr].pendingWrite = flag; };

        uint32_t GetPC() const { return pc; };
        uint64_t GetCycle() const { return cycle; };

        void Clock() {
            // Point PC to delay slot to support correct branching operation
            // pc += 4; // PC now points to delay slot
            // Instr(pc - 4).Execute(*this); // But execute the current instruction

            executors.fetch.Consume();
            executors.issue.Consume();
            executors.alu.Consume();

            executors.fetch.Produce();
            executors.issue.Produce();
            executors.alu.Produce();

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

        bool HasActiveRAW_WAW(const Instruction& instr) const {
            const auto [deps, affects] = instr.GetDeps();

            // Check RAW
            for (const auto& r : deps) {
                if (IsRegPendingWrite(r))
                    return true;
            }

            // Check WAW
            if (!affects.has_value()) return false;
            else return IsRegPendingWrite(affects.value());
        }


        static bool HasInterWAW_WAR(const Instruction& earlier, const Instruction& later) {
            const auto [eDeps, eAffects] = earlier.GetDeps();
            const auto [lDeps, lAffects] = later.GetDeps();

            if (!lAffects.has_value()) return false;

            // Check WAR
            for (const auto& r : eDeps) {
                if (r == lAffects)
                    return true;
            }

            // Check WAW
            return lAffects == eAffects;
        }

        static bool HasInterRAW_WAW_WAR(const Instruction& earlier, const Instruction& later) {
            const auto [eDeps, eAffects] = earlier.GetDeps();
            const auto [lDeps, lAffects] = later.GetDeps();

            // Check RAW
            for (const auto& r : lDeps) {
                if (eAffects == r)
                    return true;
            }

            // Check WAR
            for (const auto& r : eDeps) {
                if (r == lAffects)
                    return true;
            }

            // Check WAW
            if (!eAffects.has_value() || !lAffects.has_value()) return false;
            return lAffects.value() == eAffects.value();
        }
    };
}