#pragma once

#include "Instruction.hpp"
#include "Buffer.hpp"
#include "Execs.hpp"
#include <map>

namespace SPIMDF {
    enum class Hazard {
          RAW
        , WAW
        , WAR
    };

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
            WritebackExec writeback;
        } executors;

        CPU(uint32_t pc = 0)
        : pc(pc)
        , executors({
              FetchExec(*this)
            , IssueExec(*this)
            , ALUExec(*this)
            , MemALUExec(*this)
            , MemExec(*this) 
            , WritebackExec(*this)
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
            executors.fetch.Consume();
            executors.issue.Consume();
            executors.alu.Consume();
            executors.memALU.Consume();
            executors.mem.Consume();
            executors.writeback.Consume();

            executors.fetch.Produce();
            executors.issue.Produce();
            executors.alu.Produce();
            executors.memALU.Produce();
            executors.mem.Produce();
            executors.writeback.Produce();

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

        // Called when an instruction is either issued or flushed from pipeline
        void SetLocks(const Instruction& instr, bool flag) {
            const auto [deps, affects] = instr.GetDeps();

            for (const auto& r : deps) {
                SetRegPendingRead(r, flag);
            }

            if (affects.has_value())
                SetRegPendingWrite(affects.value(), flag);
        }

        // Called when an instruction is issued and we have to update the locks
        void AddLocks(const Instruction& instr) {
            SetLocks(instr, true);
        }

        // Called when an instruction is flushed and we have to update the locks
        void RemoveLocks(const Instruction& instr) {
            SetLocks(instr, false);
        }

        template<Hazard... Args>
        bool HasActiveHazard(const Instruction& instr) const {
            static_assert(sizeof...(Args) > 0);

            const auto [deps, affects] = instr.GetDeps();

            // Check RAW
            if constexpr (((Args == Hazard::RAW) || ...)) {
                for (const auto& r : deps) {
                    if (IsRegPendingWrite(r))
                        return true;
                }
            }

            if (!affects.has_value()) return false; // Cannot have WAW or WAR if there are no affects
                
            // Check WAW
            if constexpr (((Args == Hazard::WAW) || ...)) {
                if (IsRegPendingWrite(affects.value()))
                    return true;
            }

            // Check WAR
            if constexpr (((Args == Hazard::WAR) || ...)) {
                return IsRegPendingRead(affects.value());
                    
            }

            return false;
        }

        template<Hazard... Args>
        static bool HasInterHazard(const Instruction& earlier, const Instruction& later) {
            static_assert(sizeof...(Args) > 0);

            const auto [eDeps, eAffects] = earlier.GetDeps();
            const auto [lDeps, lAffects] = later.GetDeps();

            // Check RAW
            if constexpr (((Args == Hazard::RAW) || ...)) {
                for (const auto& r : lDeps) {
                    if (r == eAffects)
                        return true;
                }
            }

            if (!lAffects.has_value()) return false; // Cannot have WAR or WAW if there is no affects

            // Check WAR
            if constexpr (((Args == Hazard::WAR) || ...)) {
                for (const auto& r : eDeps) {
                    if (r == lAffects)
                        return true;
                }
            }

            // Check WAW
            if constexpr (((Args == Hazard::WAW) || ...)) {
                return lAffects == eAffects;
            }

            return false;
        }

        // static bool HasInterRAW_WAW_WAR(const Instruction& earlier, const Instruction& later) {
        //     const auto [eDeps, eAffects] = earlier.GetDeps();
        //     const auto [lDeps, lAffects] = later.GetDeps();

        //     // Check RAW
        //     for (const auto& r : lDeps) {
        //         if (eAffects == r)
        //             return true;
        //     }

        //     // Check WAR
        //     for (const auto& r : eDeps) {
        //         if (r == lAffects)
        //             return true;
        //     }

        //     // Check WAW
        //     if (!eAffects.has_value() || !lAffects.has_value()) return false;
        //     return lAffects.value() == eAffects.value();
        // }
    };
}