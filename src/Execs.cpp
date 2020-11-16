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
    if (numEmpty == 0) return; // Check for empty space in preissue queue
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        goto DecodedJumpOrBreak;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJumpOrBreak;

    // Set first slot
    slot1 = cpu->CurInstr();
    cpu->RelJump(4);

    // Decode for second slot
    if (numEmpty == 1) return; // Check for empty space in preissue queue
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        goto DecodedJumpOrBreak;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJumpOrBreak;

    // Set second slot
    slot2 = cpu->CurInstr();
    cpu->RelJump(4);
    return;

DecodedJumpOrBreak: // Stall if we encounter a jump instruction
    staller = cpu->CurInstr();
    cpu->RelJump(4);
    return;
}

void FetchExec::Produce() {
    // Don't need to check for empty space because we checked that in Consume().
    // Using std::move will make the instruction a noop.
    if (slot1.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(slot1)});
    if (slot2.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(slot2)});
    
    // Remove the executed instruction if it exists
    if (IsExecuted()) {
        const auto _ = std::move(executed); // Clear previously executed instruction
    }

    if (IsStalled()) {
        // Check if we are stalled. If we are, check reg status and possibly move to execution
        if (!cpu->HasActiveHazard<Hazard::RAW>(staller) && !HasStallerPreIssueHazard()) {
            staller.Execute(*cpu);
            executed = std::move(staller);
        }
    }
}

bool FetchExec::HasStallerPreIssueHazard() const {
    for (const auto& entry : cpu->queues.preIssue.entries) {
        if (!entry.has_value()) break;
        
        if (cpu->HasInterHazard<Hazard::RAW>(entry->instruction, staller))
            return true;
    }

    return false;
}

bool FetchExec::IsStalled() const {
    return staller.opcode != ISA::Opcode::NOP;
}

bool FetchExec::IsExecuted() const {
    return executed.opcode != ISA::Opcode::NOP;
}

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
        if (cpu->HasActiveHazard<Hazard::RAW, Hazard::WAW>(potentialIssue))
            continue;
     
        // Now check all previous not-issued instructions for hazards
        for (auto pit = preIssEntries.begin(); pit != it; pit++) {
            // Check RAW, WAW, WAR hazard
            if (cpu->HasInterHazard<Hazard::RAW, Hazard::WAW, Hazard::WAR>(pit->value().instruction, potentialIssue))
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
    if (instr2 != preIssEntries.end() && !CPU::HasInterHazard<Hazard::WAW, Hazard::WAR>(instr1->value().instruction, instr2->value().instruction)) 
        slot2 = preIssEntries.pull(instr2).instruction;

    if (instr1 != preIssEntries.end())
        slot1 = preIssEntries.pull(instr1).instruction;
}

void IssueExec::Produce() {
    if (!slot1.IsNop()) {
        cpu->AddLocks(slot1);

        if (slot1.IsMemAccess())
            cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(slot1) });
        else
            cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(slot1) });

        if (!slot2.IsNop()) {
            cpu->AddLocks(slot2);

            if (slot2.IsMemAccess())
                cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(slot2) });
            else
                cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(slot2) });
        }
    }
        
}

void ALUExec::Consume() {
    if (!cpu->queues.preALU.entries.is_empty())
        slot = cpu->queues.preALU.entries.pop_front().instruction;
}

void ALUExec::Produce() {
    if (slot.IsNop()) return;
    
    int32_t result = slot.ExecuteResult(*cpu);
    cpu->queues.postALU.entries.push_back(BufferEntry::PostALU{ std::move(slot), result });
}

void MemALUExec::Consume() {
    if (!cpu->queues.preMemALU.entries.is_empty())
        slot = cpu->queues.preMemALU.entries.pop_front().instruction;
}

void MemALUExec::Produce() {
    if (slot.IsNop()) return;

    uint32_t memAddr = (uint32_t) slot.ExecuteResult(*cpu);

    cpu->queues.preMem.entries.push_back(BufferEntry::PreMem{ std::move(slot), memAddr });    
}

void MemExec::Consume() {
    if (!cpu->queues.preMem.entries.is_empty())
        slot = cpu->queues.preMem.entries.pop_front();
}

void MemExec::Produce() {
    if (!slot.has_value()) return;

    if (slot->instruction.IsStore()) {
        cpu->Mem(slot->address) = cpu->Reg(slot->instruction.GetFormat<ISA::IType>().rt);
    } else if (slot->instruction.IsLoad()) {
        int32_t result = cpu->Mem(slot->address);
        cpu->queues.postMem.entries.push_back(BufferEntry::PostMem{ slot->instruction, result });
    }

    slot.reset();
}

void WritebackExec::Consume() {
    if (!cpu->queues.postALU.entries.is_empty())
        slotALU = cpu->queues.postALU.entries.pop_front();

    if (!cpu->queues.postMem.entries.is_empty())
        slotMem = cpu->queues.postMem.entries.pop_front();
}

void WritebackExec::Produce() {
    // We can assume affects has a value because it will not reach WB if it does not
    if (slotALU.has_value()) {
        const auto [deps, affects] = slotALU->instruction.GetDeps();
        cpu->Reg(affects.value()) = slotALU->result;
        
        cpu->RemoveLocks(slotALU->instruction);
        slotALU.reset();
    }

    if (slotMem.has_value()) {
        const auto [deps, affects] = slotMem->instruction.GetDeps();
        cpu->Reg(affects.value()) = slotMem->result;

        cpu->RemoveLocks(slotMem->instruction);
        slotMem.reset();
    }
}