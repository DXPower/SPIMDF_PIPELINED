#pragma once

#include <map>

namespace SPIMDF {
    class CPU;

    void Disassemble(const char* filename, CPU& cpu);
}