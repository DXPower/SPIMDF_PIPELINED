#include "Execs.hpp"
#include "Buffer.hpp"
#include "CPU.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"
#include <tuple>

using namespace SPIMDF;

void FetchExec::Consume() {
    if (IsStalled() || isBroken)
        return;
    
    // Check how many empty slots there are so we fetch the right amount
    std::size_t numEmpty = cpu->queues.preIssue.entries.num_empty();

    // Decode  for first slot
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        return;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJump;
    if (numEmpty == 0) return; // If it's not a jump, check for empty space in preissue queue

    // Set first slot
    curInstr = cpu->CurInstr();
    cpu->RelJump(4);

    // Decode for second slot
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        return;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJump;
    if (numEmpty == 1) return; // If it's not a jump, check for empty space in preissue queue

    // Set second slot
    curInstr2 = cpu->CurInstr();
    cpu->RelJump(4);
    return;

DecodedJump: // Stall if we encounter a jump instruction
    staller = cpu->CurInstr();
    cpu->RelJump(4);
    return;
}

void FetchExec::Produce() {
    if (IsStalled()) {
        // Check if we are stalled. If we are, check reg status and possibly move to execution
        staller.Execute(*cpu);
        executed = std::move(staller);
    } else if (IsExecuted()) {
        executed = Instruction::Create<ISA::NOP>(0); // Clear previously executed instruction
    }

    // Don't need to check for empty space because we checked that in Consume().
    // Using std::move will make the instruction a noop.
    if (curInstr.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(curInstr)});
    if (curInstr2.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(curInstr2)});
}

bool FetchExec::IsStalled() const {
    return staller.opcode != ISA::Opcode::NOP;
}

bool FetchExec::IsExecuted() const {
    return executed.opcode != ISA::Opcode::NOP;
}

void IssueExec::Consume() {
    
}

void IssueExec::Produce() {
    
}

void ALUExec::Consume() {
    
}

void ALUExec::Produce() {
    
}

void MemALUExec::Consume() {
    
}

void MemALUExec::Produce() {
    
}

void MemExec::Consume() {
    
}

void MemExec::Produce() {
    
}