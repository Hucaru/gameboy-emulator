#include "nlohmann/json.hpp"
#include "emulator.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <fstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    if (argc < 2) 
    {
        printf("Please provide a path to the cpu test jsons\n");
        return 0;
    }

    const auto path = std::string(argv[1]);

    printf("Starting CPU instruction tests\n");

    for (const auto &item : fs::directory_iterator(path))
    {
        if (item.path().extension() != ".json" )
        {
            continue;
        }

        std::ifstream f(item.path());
        json data = json::parse(f);

        for (const auto& entry : data)
        {
            CPU cpu;
            Memory_Bus memory;

            cpu.registers[0] = entry["initial"]["b"];
            cpu.registers[1] = entry["initial"]["c"];
            cpu.registers[2] = entry["initial"]["d"];
            cpu.registers[3] = entry["initial"]["e"];
            cpu.registers[4] = entry["initial"]["h"];
            cpu.registers[5] = entry["initial"]["l"];
            cpu.registers[6] = entry["initial"]["f"];
            cpu.registers[7] = entry["initial"]["a"];

            cpu.pc = entry["initial"]["pc"] - 1;
            cpu.sp = entry["initial"]["sp"];

            for (const auto& mem : entry["initial"]["ram"])
            {
                memory.memory[mem[0]] = mem[1];
            }

            std::cout << "Instruction: " << entry["name"] << "\n";
            cpu_cycle(&cpu, &memory);

            bool pass = true;

            if (cpu.registers[0] != entry["final"]["b"])
            {
                std::cout << "register B does not have expected value "  << (u16)cpu.registers[0] << " != " << entry["final"]["b"] << std::endl;
                pass = false;
            }

            if (cpu.registers[1] != entry["final"]["c"])
            {
                std::cout << "register B does not have expected value "  << (u16)cpu.registers[1] << " != " << entry["final"]["c"] << std::endl;
                pass = false;
            }

            if (cpu.registers[2] != entry["final"]["d"])
            {
                std::cout << "register C does not have expected value "  << (u16)cpu.registers[2] << " != " << entry["final"]["d"] << std::endl;
                pass = false;
            }

            if (cpu.registers[3] != entry["final"]["e"])
            {
                std::cout << "register D does not have expected value "  << (u16)cpu.registers[3] << " != " << entry["final"]["e"] << std::endl;
                pass = false;
            }

            if (cpu.registers[4] != entry["final"]["h"])
            {
                std::cout << "register H does not have expected value "  << (u16)cpu.registers[4] << " != " << entry["final"]["h"] << std::endl;
                pass = false;
            }

            if (cpu.registers[5] != entry["final"]["l"])
            {
                std::cout << "register L does not have expected value "  << (u16)cpu.registers[5] << " != " << entry["final"]["l"] << std::endl;
                pass = false;
            }

            if (cpu.registers[6] != entry["final"]["f"])
            {
                std::cout << "register F does not have expected value "  << (u16)cpu.registers[6] << " != " << entry["final"]["f"] << std::endl;
                pass = false;
            }

            if (cpu.registers[7] != entry["final"]["a"])
            {
                std::cout << "register A does not have expected value " << (u16)cpu.registers[7] << " != " << entry["final"]["a"] << std::endl;
                pass = false;
            }

            if (cpu.pc != entry["final"]["pc"] - 1)
            {
                std::cout << "program counter is incorrect " << cpu.pc << " != " << entry["final"]["pc"] - 1 << std::endl;
                pass = false;
            }

            if (cpu.sp != entry["final"]["sp"])
            {
                std::cout << "stack pointer is incorrect " << cpu.sp << " != " << entry["final"]["sp"] << std::endl;
                pass = false;
            }

            for (const auto& mem : entry["final"]["ram"])
            {
                if (memory.memory[mem[0]] != mem[1])
                {
                    std::cout << "memory is incorrect " << (u16)memory.memory[mem[0]] << " != " << (u16)mem[1] << std::endl;
                    pass = false;
                }
            }

            if (!pass)
            {
                std::cout << "CPU test failed" << std::endl;
                return 0;
            }

        }

        f.close();
    }

    
    std::cout << "CPU passed all instruction tests" << std::endl;
    
    return 0;
}