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

// bool IssueExec::MapActiveInstructions(const std::function<bool(const Instruction&)>& func) const {
//     // #define LoopBuffer(q) for (const auto& entry : q.entries) { \
//     //     if (!entry.has_value()) break; \
//     //     if (!func(entry.value().instruction)) return false; \
//     // }

//     // LoopBuffer(cpu->queues.preALU);
//     // LoopBuffer(cpu->queues.postALU);
//     // LoopBuffer(cpu->queues.preMemALU);
//     // LoopBuffer(cpu->queues.preMem);
//     // LoopBuffer(cpu->queues.postMem);

//     // return true;
// }


void IssueExec::Consume() {
    auto& preIssEntries = cpu->queues.preIssue.entries;
    
    auto instr1 = preIssEntries.end();
    auto instr2 = preIssEntries.end();

    for (auto it = preIssEntries.begin(); it != preIssEntries.end(); it++) {
        auto& entry = *it;

        if (!entry.has_value()) break;
        
        Instruction& potentialIssue = entry.value().instruction;

        // Check for structural hazard with existing instructions in PreALU and PreMemALU
        if (potentialIssue.IsMemAccess() && cpu->queues.preMemALU.entries.is_full())
            continue;
        
        if (!potentialIssue.IsMemAccess() && cpu->queues.preALU.entries.is_full())
            continue;

        // Check if RAW or WAW hazard exists on active instructions (anything issued but not finished)
        if (cpu->HasActiveRAW_WAW(potentialIssue))
            continue;
     
        // Now check all previous not-issued instructions for hazards
        for (auto pit = preIssEntries.begin(); pit != it; pit++) {
            // Check RAW, WAW, WAR hazard
            if (cpu->HasInterRAW_WAW_WAR(pit->value().instruction, potentialIssue))
                goto SKIP_INSTR; // continue outer loop

            // Make sure all loads only happen once previous stores have been issued (if it potentialIssue is a load)
            // Additionally make sure any stores happen in order (if potentialIssue is a store)
            if (potentialIssue.IsMemAccess() && pit->value().instruction.IsStore())
                goto SKIP_INSTR; // continue outer loop
        }

        // No hazard, select this instruction.
        // But, we can't change the array because we are iterating through it
        if (instr1 == preIssEntries.end()) {
            instr1 = it;
        } else if (instr2 == preIssEntries.end()) {
            // Now we need to check if this second instruction will have a structural hazard with the first instruction selected
            if (instr1->value().instruction.IsMemAccess() == potentialIssue.IsMemAccess()) // If they are the same type of "access", then hazard
                continue;

            instr2 = it;
            break; // Maximum of 2 instructions.
        }

        SKIP_INSTR: // For continuing outer loop from inner loop
        ; // Noop
    }

    // Continue with the selected instructions
    // Do second instruction first so that the iterator positions are not invalidated
    //TODO: Check if HasInterWAW_WAR here is necessary
    if (instr2 != preIssEntries.end() && !CPU::HasInterWAW_WAR(instr1->value().instruction, instr2->value().instruction)) 
        curInstr2 = preIssEntries.pull(instr2).instruction;

    if (instr1 != preIssEntries.end())
        curInstr = preIssEntries.pull(instr1).instruction;
}

void IssueExec::Produce() {
    if (!curInstr.IsNop()) {
        if (curInstr.IsMemAccess())
            cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(curInstr) });
        else
            cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(curInstr) });

        if (!curInstr2.IsNop()) {
            if (curInstr2.IsMemAccess())
                cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(curInstr2) });
            else
                cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(curInstr2) });
        }
    }
        
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