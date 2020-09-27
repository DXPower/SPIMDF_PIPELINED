#include <cstdio>
#include "Disassembler.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"

using namespace SPIMDF;

using voidFunc = void();

struct Foo {
    int x;
    int y;
};

struct Foo foo = { .x = 2, .y = 3 };

int main() {
    SPIMDF::Disassemble("sample.txt");
    Instruction jumper0 = Instruction::Create<ISA::J>(120);
    // Instruction jumper1 = Instruction::Create<ISA::J>(123);
    // Instruction jumper2 = Instruction::Create<ISA::J>(129);

    jumper0.Print();
    // jumper1.Print();
    // jumper2.Print();

    // jumper2 = jumper0;
    // jumper0 = Instruction::Create<ISA::J>(117);

    // jumper0.Print();
    // jumper1.Print();
    // jumper2.Print();

    // auto adder = Instruction::Create<ISA::ADD>(20, 30, 10);
    // auto adderI = Instruction::Create<ISA::ADDI>(21, 150, 20);

    // adder.Print();
    // adderI.Print();
    // adder.Execute();
    // adderI.Execute();


    // adder = Instruction::Create<ISA::SUB>(51, 52, 53);
    // adderI = Instruction::Create<ISA::BEQ>(20, 10, 120);

    // adder.Print();
    // adderI.Print();

    // jumper0 = Instruction::Create<ISA::JR>(20);
    // jumper0.Print();

    // auto bltz = Instruction::Create<ISA::BLTZ>(20, 430);
    // bltz.Print();

}