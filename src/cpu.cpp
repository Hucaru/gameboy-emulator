#include "emulator.h"
#include <cstdio>

constexpr i8 ZERO_FLAG_POS = 7;
constexpr i8 SUBTRACT_FLAG_POS = 6;
constexpr i8 HALF_CARRY_FLAG_POS = 5;
constexpr i8 CARRY_FLAG_POS = 4;

namespace Register
{
    enum
    {
        B = 0 ,C,D,E,H,L,F,A
    };
}

u8
flag_zero(CPU *cpu)
{
    return (cpu->registers[Register::F] >> ZERO_FLAG_POS) & 0x01;
}

void
set_flag_zero(CPU *cpu, bool v)
{
    if (v)
    {
        cpu->registers[Register::F] |= (0x01 << ZERO_FLAG_POS);
    }
    else
    {
        cpu->registers[Register::F] &= ~(0x01 << ZERO_FLAG_POS);
    }
    
}

u8
flag_subtract(CPU *cpu)
{
    return (cpu->registers[Register::F] >> SUBTRACT_FLAG_POS) & 0x01;
}

void
set_flag_subtract(CPU *cpu, bool v)
{
    if (v)
    {
        cpu->registers[Register::F] |= (0x01 << SUBTRACT_FLAG_POS);
    }
    else
    {
        cpu->registers[Register::F] &= ~(0x01 << SUBTRACT_FLAG_POS);
    }
    
}

u8
flag_half_carry(CPU *cpu)
{
    return (cpu->registers[Register::F] >> HALF_CARRY_FLAG_POS) & 0x01;
}

void
set_flag_half_carry(CPU *cpu, bool v)
{
    if (v)
    {
        cpu->registers[Register::F] |= (0x01 << HALF_CARRY_FLAG_POS);
    }
    else
    {
        cpu->registers[Register::F] &= ~(0x01 << HALF_CARRY_FLAG_POS);
    }    
}

u8
flag_carry(CPU *cpu)
{
    return (cpu->registers[Register::F] >> CARRY_FLAG_POS) & 0x01;
}

void
set_flag_carry(CPU *cpu, bool v)
{
    if (v)
    {
        cpu->registers[Register::F] |= (0x01 << CARRY_FLAG_POS);
    }
    else
    {
        cpu->registers[Register::F] &= ~(0x01 << CARRY_FLAG_POS);
    }
    
}

u16 
register_af(CPU *cpu)
{
    return static_cast<u16>(cpu->registers[Register::A]) << 8 | static_cast<u16>(cpu->registers[Register::F]);
}

void 
set_register_af(CPU *cpu, u16 val)
{
    cpu->registers[Register::A] = (val & 0xFF00) >> 8;
    cpu->registers[Register::F] = val & 0xF0; // only interested in the flag bits
}

u16 
register_bc(CPU *cpu)
{
    return static_cast<u16>(cpu->registers[Register::B]) << 8 | static_cast<u16>(cpu->registers[Register::C]);
}

void 
set_register_bc(CPU *cpu, u16 val)
{
    cpu->registers[Register::B] = (val & 0xFF00) >> 8;
    cpu->registers[Register::C] = val & 0xFF;
}

u16 
register_de(CPU *cpu)
{
    return static_cast<u16>(cpu->registers[Register::D]) << 8 | static_cast<u16>(cpu->registers[Register::E]);
}

void 
set_register_de(CPU *cpu, u16 val)
{
    cpu->registers[Register::D] = (val & 0xFF00) >> 8;
    cpu->registers[Register::E] = val & 0xFF;
}

u16 
register_hl(CPU *cpu)
{
    return static_cast<u16>(cpu->registers[Register::H]) << 8 | static_cast<u16>(cpu->registers[Register::L]);
}

void 
set_register_hl(CPU *cpu, u16 val)
{
    cpu->registers[Register::H] = (val & 0xFF00) >> 8;
    cpu->registers[Register::L] = val & 0xFF;
}

u8 
adc(CPU *cpu, u8 dst, u8 src, u8 flag)
{
    u16 result = dst + src + flag;

    set_flag_zero(cpu, static_cast<u8>(result) == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, ((dst & 0x0F) + (src & 0x0F) + flag) & 0x10);
    set_flag_carry(cpu, result > 0xff);

    return static_cast<u8>(result);
}

u16 
add_u16_i8(CPU *cpu, u16 dst, i8 src)
{
    i32 result = dst + src;

    set_flag_zero(cpu, false);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, (dst ^ src ^ (result & 0xFFFF)) & 0x10);
    set_flag_carry(cpu, (dst ^ src ^ (result & 0xFFFF)) & 0x100);

    return static_cast<u16>(result);
}

u16 
add_registers_u16(CPU *cpu, u16 dst, u16 src)
{
    u32 result = dst + src;
    
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, ((dst & 0xFFF) + (src & 0xFFF)) > 0xFFF);
    set_flag_carry(cpu, (result & 0x10000) != 0);

    return static_cast<u16>(result);
}

u8
sbc(CPU *cpu, u8 dst, u8 src, u8 flag)
{
    i16 result = dst - src - flag;

    set_flag_zero(cpu, static_cast<u8>(result) == 0);
    set_flag_subtract(cpu, true);
    set_flag_half_carry(cpu, ((dst & 0x0F) - (src & 0x0F) - flag) & 0x10);
    set_flag_carry(cpu, result < 0);

    return static_cast<u8>(result);
}

u8
inc_u8(CPU *cpu, u8 dst)
{
    i16 result = dst + 1;

    set_flag_zero(cpu, static_cast<u8>(result) == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, ((dst & 0x0F) + 1) & 0x10);

    return static_cast<u8>(result);
}

u8
dec_u8(CPU *cpu, u8 dst)
{
    i16 result = dst - 1;

    set_flag_zero(cpu, static_cast<u8>(result) == 0);
    set_flag_subtract(cpu, true);
    set_flag_half_carry(cpu, ((dst & 0x0F) - 1) & 0x10);

    return static_cast<u8>(result);
}

u8
bitwise_and(CPU *cpu, u8 dst, u8 src)
{
    u8 result = dst & src;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, true);
    set_flag_carry(cpu, false);

    return result;
}

u8
bitwise_xor(CPU *cpu, u8 dst, u8 src)
{
    u8 result = dst ^ src;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, false);

    return result;
}

u8
bitwise_or(CPU *cpu, u8 dst, u8 src)
{
    u8 result = dst | src;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, false);

    return result;
}

// Rotate the val left over the carry flag
u8
rlc(CPU *cpu, u8 val)
{
    u8 truncated_bit = (val >> 7) & 0x01;
    u8 result = (val << 1) | truncated_bit;
    
    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, truncated_bit == 1);
    
    return result;
}

// Rotate the val left through the carry flag
u8
rl(CPU *cpu, u8 val)
{
    u8 carry_bit = flag_carry(cpu);
    u8 truncated_bit = (val >> 7) & 0x01;
    u8 result = (val << 1) | carry_bit;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, truncated_bit == 1);
    
    return result;
}

// Rotate the register right over the carry flag
u8
rrc(CPU *cpu, u8 val)
{
    u8 truncated_bit = (val & 0x01);
    u8 result = (val >> 1) | (truncated_bit << 7);

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, truncated_bit == 1);
    
    return result;
}

// Rotate the register right through the carry flag
u8
rr(CPU *cpu, u8 val)
{
    u8 carry_bit = flag_carry(cpu);
    u8 truncated_bit = (val & 0x01);
    u8 result = (val >> 1) | (carry_bit << 7);

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, truncated_bit == 1);
    
    return result;
}

u8 
sla(CPU *cpu, u8 val) 
{
    u8 result = val << 1;
    
    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, val & 0x80);
    
    return result;
}

u8 
sra(CPU *cpu, u8 val)
{
    u8 static_bit = val & 0x80;
    u8 result = (val >> 1) | static_bit;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, val & 0x01);
    
    return result;
}

u8 
srl(CPU *cpu, u8 val)
{
    u8 result = val >> 1;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, val & 0x01);

    return result;
}

u8 
swap(CPU *cpu, u8 val)
{
    u8 high = (val >> 4) & 0x0F ;
    u8 low = (val << 4) & 0xF0;
    u8 result = low | high;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, false);
    set_flag_carry(cpu, false);
    
    return result;
}

void 
bit(CPU *cpu, u8 val, u8 bit_field)
{
    u8 result = (val >> bit_field) & 0x01;

    set_flag_zero(cpu, result == 0);
    set_flag_subtract(cpu, false);
    set_flag_half_carry(cpu, true);
    // NOTE: Carry flag is unmodified
}

u8 
res(CPU *cpu, u8 val, u8 bit_field)
{
    u8 bit = 0xFF ^ (1 << bit_field);
    return val & bit;
}

u8 
set(CPU *cpu, u8 val, u8 bit_field)
{
    return val | (1 << bit_field);
}

void
stack_pop(CPU *cpu, Memory_Bus *memory_bus)
{
    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->w = memory_bus->read_u8(cpu->sp++);
    });

    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->z = memory_bus->read_u8(cpu->sp++);
    });
}

void
set_pc_from_tmp_2m(CPU *cpu)
{
    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->pc = static_cast<u16>(cpu->w);
    });

    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->pc |= static_cast<u16>(cpu->z) << 8;
    });
}

void
set_pc_from_tmp_1m(CPU *cpu)
{
    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->pc = static_cast<u16>(cpu->w) | static_cast<u16>(cpu->z) << 8;
    });
}

void 
stack_push(CPU *cpu, Memory_Bus *memory_bus, u16 val)
{
    cpu->pipeline.push([val](CPU *cpu, Memory_Bus *memory_bus)
    {
        memory_bus->write_u8(--cpu->sp, (val >> 8) & 0xFF);
    });

    cpu->pipeline.push([val](CPU *cpu, Memory_Bus *memory_bus)
    {
        memory_bus->write_u8(--cpu->sp, val & 0xFF);
    });
}

void
set_pc_from_mem_pc(CPU *cpu, Memory_Bus *memory_bus)
{
    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->w = memory_bus->read_u8(cpu->pc++);
    });

    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->z = memory_bus->read_u8(cpu->pc++);
    });

    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
    {
        cpu->pc = static_cast<u16>(cpu->w) | static_cast<u16>(cpu->z) << 8;
    });
}

void 
handle_extended_opcode(CPU *cpu, Memory_Bus *memory_bus, u8 opcode)
{
    u8 src = opcode & 0x07;
    u8 second_val = (opcode >> 3) & 0x07;

    switch (opcode)
    {
        // RLC B ; RLC C ; RLC D ; RLC E ; RLC H ; RLC L ; RLC (HL) ; RLC A
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = rlc(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = rlc(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // RRC B ; RRC C ; RRC D ; RRC E ; RRC H ; RRC L ; RRC (HL) ; RRC A
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = rrc(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = rrc(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // RL B ; RL C ; RL D ; RL E ; RL H ; RL L ; RL (HL) ; RL A
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = rl(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = rl(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // RR B ; RR C ; RR D ; RR E ; RR H ; RR L ; RR (HL) ; RR A
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = rr(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = rr(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // SLA B ; SLA C ; SLA D ; SLA E ; SLA H ; SLA L ; SLA (HL) ; SLA A
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = sla(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = sla(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // SRA B ; SRA C ; SRA D ; SRA E ; SRA H ; SRA L ; SRA (HL) ; SRA A
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = sra(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = sra(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // SWAP B ; SWAP C ; SWAP D ; SWAP E ; SWAP H ; SWAP L ; SWAP (HL) ; SWAP A
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = swap(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = swap(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // SRL B ; SRL C ; SRL D ; SRL E ; SRL H ; SRL L ; SRL (HL) ; SRL A
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = srl(cpu, cpu->w);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = srl(cpu, cpu->registers[src]);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // BIT 0, B ; BIT 0, C ; BIT 0, D ; BIT 0, E ; BIT 0, H ; BIT 0, L ; BIT 0, (HL) ; BIT 0, A
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        // BIT 1, B ; BIT 1, C ; BIT 1, D ; BIT 1, E ; BIT 1, H ; BIT 1, L ; BIT 1, (HL) ; BIT 1, A
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        // BIT 2, B ; BIT 2, C ; BIT 2, D ; BIT 2, E ; BIT 2, H ; BIT 2, L ; BIT 2, (HL) ; BIT 2, A
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        // BIT 3, B ; BIT 3, C ; BIT 3, D ; BIT 3, E ; BIT 3, H ; BIT 3, L ; BIT 3, (HL) ; BIT 3, A
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        // BIT 4, B ; BIT 4, C ; BIT 4, D ; BIT 4, E ; BIT 4, H ; BIT 4, L ; BIT 4, (HL) ; BIT 4, A
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        // BIT 5, B ; BIT 5, C ; BIT 5, D ; BIT 5, E ; BIT 5, H ; BIT 5, L ; BIT 5, (HL) ; BIT 5, A
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        // BIT 6, B ; BIT 6, C ; BIT 6, D ; BIT 6, E ; BIT 6, H ; BIT 6, L ; BIT 6, (HL) ; BIT 6, A
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        // BIT 7, B ; BIT 7, C ; BIT 7, D ; BIT 7, E ; BIT 7, H ; BIT 7, L ; BIT 7, (HL) ; BIT 7, A
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                });

                cpu->pipeline.push([second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    bit(cpu, cpu->w, second_val);
                });
            }
            else
            {
                cpu->pipeline.push([src, second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    bit(cpu, cpu->registers[src], second_val);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // RES 0, B ; RES 0, C ; RES 0, D ; RES 0, E ; RES 0, H ; RES 0, L ; RES 0, (HL) ; RES 0, A
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        // RES 1, B ; RES 1, C ; RES 1, D ; RES 1, E ; RES 1, H ; RES 1, L ; RES 1, (HL) ; RES 1, A
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F:
        // RES 2, B ; RES 2, C ; RES 2, D ; RES 2, E ; RES 2, H ; RES 2, L ; RES 2, (HL) ; RES 2, A
        case 0x90:
        case 0x91:
        case 0x92:
        case 0x93:
        case 0x94:
        case 0x95:
        case 0x96:
        case 0x97:
        // RES 3, B ; RES 3, C ; RES 3, D ; RES 3, E ; RES 3, H ; RES 3, L ; RES 3, (HL) ; RES 3, A
        case 0x98:
        case 0x99:
        case 0x9A:
        case 0x9B:
        case 0x9C:
        case 0x9D:
        case 0x9E:
        case 0x9F:
        // RES 4, B ; RES 4, C ; RES 4, D ; RES 4, E ; RES 4, H ; RES 4, L ; RES 4, (HL) ; RES 4, A
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA6:
        case 0xA7:
        // RES 5, B ; RES 5, C ; RES 5, D ; RES 5, E ; RES 5, H ; RES 5, L ; RES 5, (HL) ; RES 5, A
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xAF:
        // RES 6, B ; RES 6, C ; RES 6, D ; RES 6, E ; RES 6, H ; RES 6, L ; RES 6, (HL) ; RES 6, A
        case 0xB0:
        case 0xB1:
        case 0xB2:
        case 0xB3:
        case 0xB4:
        case 0xB5:
        case 0xB6:
        case 0xB7:
        // RES 7, B ; RES 7, C ; RES 7, D ; RES 7, E ; RES 7, H ; RES 7, L ; RES 7, (HL) ; RES 7, A
        case 0xB8:
        case 0xB9:
        case 0xBA:
        case 0xBB:
        case 0xBC:
        case 0xBD:
        case 0xBE:
        case 0xBF:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = res(cpu, cpu->w, second_val);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src, second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = res(cpu, cpu->registers[src], second_val);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
        // SET 0, B ; SET 0, C ; SET 0, D ; SET 0, E ; SET 0, H ; SET 0, L ; SET 0, (HL) ; SET 0, A
        case 0xC0:
        case 0xC1:
        case 0xC2:
        case 0xC3:
        case 0xC4:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        // SET 1, B ; SET 1, C ; SET 1, D ; SET 1, E ; SET 1, H ; SET 1, L ; SET 1, (HL) ; SET 1, A
        case 0xC8:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCC:
        case 0xCD:
        case 0xCE:
        case 0xCF:
        // SET 2, B ; SET 2, C ; SET 2, D ; SET 2, E ; SET 2, H ; SET 2, L ; SET 2, (HL) ; SET 2, A
        case 0xD0:
        case 0xD1:
        case 0xD2:
        case 0xD3:
        case 0xD4:
        case 0xD5:
        case 0xD6:
        case 0xD7:
        // SET 3, B ; SET 3, C ; SET 3, D ; SET 3, E ; SET 3, H ; SET 3, L ; SET 3, (HL) ; SET 3, A
        case 0xD8:
        case 0xD9:
        case 0xDA:
        case 0xDB:
        case 0xDC:
        case 0xDD:
        case 0xDE:
        case 0xDF:
        // SET 4, B ; SET 4, C ; SET 4, D ; SET 4, E ; SET 4, H ; SET 4, L ; SET 4, (HL) ; SET 4, A
        case 0xE0:
        case 0xE1:
        case 0xE2:
        case 0xE3:
        case 0xE4:
        case 0xE5:
        case 0xE6:
        case 0xE7:
        // SET 5, B ; SET 5, C ; SET 5, D ; SET 5, E ; SET 5, H ; SET 5, L ; SET 5, (HL) ; SET 5, A
        case 0xE8:
        case 0xE9:
        case 0xEA:
        case 0xEB:
        case 0xEC:
        case 0xED:
        case 0xEE:
        case 0xEF:
        // SET 6, B ; SET 6, C ; SET 6, D ; SET 6, E ; SET 6, H ; SET 6, L ; SET 6, (HL) ; SET 6, A
        case 0xF0:
        case 0xF1:
        case 0xF2:
        case 0xF3:
        case 0xF4:
        case 0xF5:
        case 0xF6:
        case 0xF7:
        // SET 7, B ; SET 7, C ; SET 7, D ; SET 7, E ; SET 7, H ; SET 7, L ; SET 7, (HL) ; SET 7, A
        case 0xF8:
        case 0xF9:
        case 0xFA:
        case 0xFB:
        case 0xFC:
        case 0xFD:
        case 0xFE:
        case 0xFF:
            if (src == 0x06)
            {
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                });

                cpu->pipeline.push([second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = set(cpu, cpu->w, second_val);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                });
            }
            else
            {
                cpu->pipeline.push([src, second_val](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[src] = set(cpu, cpu->registers[src], second_val);
                });
            }

            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
            break;
    }
}

void
handle_opcode(CPU *cpu, Memory_Bus *memory_bus, u8 opcode)
{
    switch(opcode)
        {
            case 0x00: // NOP
                break;
            case 0x01: // LD BC, u16
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::C] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::B] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x02: // LD (BC), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_bc(cpu), cpu->registers[Register::A]);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x03: // INC BC
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_bc(cpu, register_bc(cpu) + 1);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x06: // LD B, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::B] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x07: // RLCA
                cpu->registers[Register::A] = rlc(cpu, cpu->registers[Register::A]);
                set_flag_zero(cpu, false);
                break;
            case 0x08: // LD (u16), SP
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->z = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(cpu->w, cpu->sp & 0xFF);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(cpu->z, (cpu->sp >> 8) & 0xFF);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x09: // ADD HL, BC
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, add_registers_u16(cpu, register_hl(cpu), register_bc(cpu)));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x0A: // LD A, (BC)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u16(register_bc(cpu));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x0B: // DEC BC
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_bc(cpu, register_bc(cpu) - 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x0E: // LD C, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::C] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x0F: // RRCA
                cpu->registers[Register::A] = rrc(cpu, cpu->registers[Register::A]);
                set_flag_zero(cpu, false);
                break;
            case 0x10: // STOP
                // TODO: stop stuff
                break;
            case 0x11: // LD DE, u16
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::E] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::D] = memory_bus->read_u8(cpu->pc++);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x012: // LD (DE), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(register_de(cpu), cpu->registers[Register::A]);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x13: // INC DE
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_de(cpu, register_de(cpu) + 1);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x17: // RLA
                cpu->registers[Register::A] = rl(cpu, cpu->registers[Register::A]);
                set_flag_zero(cpu, false);
                break;
            case 0x18: // JR i8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc += cpu->w;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x19: // ADD HL, DE
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, add_registers_u16(cpu, register_hl(cpu), register_de(cpu)));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x1A: // LD A, (DE)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] =  memory_bus->read_u16(register_de(cpu));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x1B: // DEC DE
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_de(cpu, register_de(cpu) - 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x1F: // RRA
                cpu->registers[Register::A] = rr(cpu, cpu->registers[Register::A]);
                set_flag_zero(cpu, false);
                break;
            case 0x20: // JR NZ, i8
                if (flag_zero(cpu))
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->w = memory_bus->read_u8(cpu->pc++);
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc += cpu->w;
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x21: // LD HL, u16
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::L] = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::H] = memory_bus->read_u8(cpu->pc++);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x22: // LD (HL++), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u16(register_hl(cpu), cpu->registers[Register::A]);
                    set_register_hl(cpu, register_hl(cpu) + 1);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x23: // INC HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, register_hl(cpu) + 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x27: // DAA
            {
                u8 val = cpu->registers[Register::A];
                u16 correction = flag_carry(cpu) ? 0x60 : 0x00;

                if (flag_half_carry(cpu) || ( !flag_subtract(cpu) && ((val & 0x0F) > 9)) )
                {
                    correction |= 0x06;
                }

                if (flag_carry(cpu) || ( !flag_subtract(cpu) && (val > 0x99)) )
                {
                    correction |= 0x60;
                }

                if (flag_subtract(cpu))
                {
                    val -= correction;
                }
                else
                {
                    val += correction;
                }

                if ( ((correction << 2) & 0x100) != 0 )
                {
                    set_flag_carry(cpu, true);
                }

                set_flag_zero(cpu, val == 0);
                set_flag_half_carry(cpu, false);
                
                cpu->registers[Register::A] = val;
            } break;
            case 0x28: // JR Z, i8
                if (flag_zero(cpu))
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->w = memory_bus->read_u8(cpu->pc++);
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc += cpu->w;
                    });
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x29: // ADD HL, HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, add_registers_u16(cpu, register_hl(cpu), register_hl(cpu)));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x2A: // LD A, (HL++)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u8(register_hl(cpu));
                    set_register_hl(cpu, register_hl(cpu) + 1);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x2B: // DEC HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, register_hl(cpu) - 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x2F: // CPL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    u8 val = cpu->registers[Register::A];
                    val = ~val;

                    set_flag_subtract(cpu, true);
                    set_flag_half_carry(cpu, true);

                    cpu->registers[Register::A] = val;
                    cpu->remaining_cycles = 4;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x30: // JR NC, i8
                if (flag_carry(cpu))
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->w = memory_bus->read_u8(cpu->pc++);
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc += cpu->w;    
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x31: // LD SP, u16
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->sp = static_cast<u16>(cpu->w) | static_cast<u16>(memory_bus->read_u8(cpu->pc++)) << 8;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x32: // LD (HL--), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u16(register_hl(cpu), cpu->registers[Register::A]);
                    set_register_hl(cpu, register_hl(cpu) - 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x33: // INC SP
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->sp++;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x37: // SCF
                set_flag_subtract(cpu, false);
                set_flag_half_carry(cpu, false);
                set_flag_carry(cpu, true);
                break;
            case 0x38: // JR C, i8
                if (flag_carry(cpu))
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->w = memory_bus->read_u8(cpu->pc++);
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc += cpu->w;
                    });
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x39: // ADD HL, SP
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    set_register_hl(cpu, add_registers_u16(cpu, register_hl(cpu), cpu->sp));
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x3A: // LD A, (HL--)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u8(register_hl(cpu));
                    set_register_hl(cpu, register_hl(cpu) - 1);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x3B: // DEC SP
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->sp--;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0x3F: // CCF
                set_flag_subtract(cpu, false);
                set_flag_half_carry(cpu, false);
                set_flag_carry(cpu, !flag_carry(cpu));
                break;
            case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: // LD
            case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
            case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
            case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
            case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: 
            case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F: 
            case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: /* case 0x76: // HALT implemented below */ case 0x77: 
            case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: 
            {
                u8 reg_src = opcode & 0x07;
                u8 reg_dst = (opcode >> 3) & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([reg_dst](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->registers[reg_dst] = memory_bus->read_u8(register_hl(cpu));
                    });
                    
                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else if (reg_dst == 6)
                {
                    cpu->pipeline.push([reg_src](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        memory_bus->write_u8(register_hl(cpu), cpu->registers[reg_src]);
                    });
                    
                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    cpu->registers[reg_dst] = cpu->registers[reg_src];
                }
            } break;
            case 0x76: // HALT
                cpu->halted = true;
                break;
            case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: // ADD
            {
                u8 reg_src = opcode & 0x07;
                
                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], val, 0);
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], val, 0);
                }
            } break;
            case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: // ADC
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], val, flag_carry(cpu));
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], val, flag_carry(cpu));
                }
            } break;
            case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: // SUB
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], val, 0);
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], val, 0);
                }
            } break;
            case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: // SBC
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], val, flag_carry(cpu));
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], val, flag_carry(cpu));
                }
            } break;
            case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: // AND
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = bitwise_and(cpu, cpu->registers[Register::A], val);
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = bitwise_and(cpu, cpu->registers[Register::A], val);
                }
            } break;
            case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: // XOR
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = bitwise_xor(cpu, cpu->registers[Register::A], val);
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = bitwise_xor(cpu, cpu->registers[Register::A], val);    
                }
            } break;
            case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: // OR
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        cpu->registers[Register::A] = bitwise_or(cpu, cpu->registers[Register::A], val);
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    cpu->registers[Register::A] = bitwise_or(cpu, cpu->registers[Register::A], val);
                }
            } break;
            case 0xB8: case 0xB9:  case 0xBA:  case 0xBB:  case 0xBC:  case 0xBD:  case 0xBE:  case 0xBF: // CP
            {
                u8 reg_src = opcode & 0x07;

                if (reg_src == 6)
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        u8 val = memory_bus->read_u8(register_hl(cpu));
                        sbc(cpu, cpu->registers[Register::A], val, 0); // ignore the result we only want to set flags
                    });

                    cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                }
                else
                {
                    u8 val = cpu->registers[reg_src];
                    sbc(cpu, cpu->registers[Register::A], val, 0); // ignore the result we only want to set flags
                }
            } break;
            case 0xC0: // RET NZ
                if (!flag_zero(cpu))
                {
                    stack_pop(cpu, memory_bus);
                    set_pc_from_tmp_2m(cpu);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC1: // POP BC
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::C] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::B] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC2: // JP NZ,u16
                if (!flag_zero(cpu))
                {
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC3: // JP u16
                set_pc_from_mem_pc(cpu, memory_bus);
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC4: // CALL NZ,u16
                if (!flag_zero(cpu))
                {
                    stack_push(cpu, memory_bus, cpu->pc + 2);
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });

                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                    {
                        cpu->pc++;
                    });
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC5: // PUSH BC
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){}); //docs say this instruction is 16 cycles
                stack_push(cpu, memory_bus, register_bc(cpu));
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC6: // ADD A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc), 0);
                    cpu->pc++;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC7: // RST 00h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x00;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC8: // RET Z
                if (flag_zero(cpu))
                {
                    stack_pop(cpu, memory_bus);
                    set_pc_from_tmp_2m(cpu);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xC9: // RET
                stack_pop(cpu, memory_bus);
                set_pc_from_tmp_1m(cpu);
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xCA: // JP Z, u16
                if (flag_zero(cpu))
                {
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xCB:
                cpu->extended = true;
                break;
            case 0xCC: // CALL Z, u16
                if (flag_zero(cpu))
                {
                    stack_push(cpu, memory_bus, cpu->pc + 2);
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xCD: // CALL u16
                stack_push(cpu, memory_bus, cpu->pc + 2);
                set_pc_from_mem_pc(cpu, memory_bus);
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xCE: // ADC A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = adc(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc), flag_carry(cpu));
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xCF: // RST 08h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x08;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD0: // RET NC
                if (!flag_carry(cpu))
                {
                    stack_pop(cpu, memory_bus);
                    set_pc_from_tmp_2m(cpu);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD1: // POP DE
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::E] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::D] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD2: // JP NC, u16
                if (!flag_carry(cpu))
                {
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD4: // CALL NC, u16
                if (!flag_carry(cpu))
                {
                    stack_push(cpu, memory_bus, cpu->pc + 2);
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD5: // PUSH DE
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                stack_push(cpu, memory_bus, register_de(cpu));
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break; 
            case 0xD6: // SUB A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc), 0);
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD7: // RST 10h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x10;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD8: // RET C
                if (flag_carry(cpu))
                {
                    stack_pop(cpu, memory_bus);
                    set_pc_from_tmp_2m(cpu);
                    cpu->remaining_cycles = 20; // with branch
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xD9: // RETI
                stack_pop(cpu, memory_bus);
                set_pc_from_tmp_1m(cpu);
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                cpu->interrupt_master_enable = true;
                break;
            case 0xDA: // JP C, u16
                if (flag_carry(cpu))
                {
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xDC: // CALL C, u16
                if (flag_carry(cpu))
                {
                    stack_push(cpu, memory_bus, cpu->pc + 2);
                    set_pc_from_mem_pc(cpu, memory_bus);
                }
                else
                {
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                    cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                }

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xDE: // SBC A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = sbc(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc), flag_carry(cpu));
                    cpu->pc++;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xDF: // RST 18h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x18;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE0: // LD (FF00+u8), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(0xFF00 + memory_bus->read_u8(cpu->pc), cpu->registers[Register::A]);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE1: // POP HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::L] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::H] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE2: // LD (FF00+C), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    memory_bus->write_u8(0xFF00 + cpu->registers[Register::C], cpu->registers[Register::A]);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE5: // PUSH HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                stack_push(cpu, memory_bus, register_hl(cpu));
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE6: // AND A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = bitwise_and(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc));
                    cpu->pc++;
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE7: // RST 20h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x20;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE8: // ADD SP, i8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->sp = add_u16_i8(cpu, cpu->sp, cpu->w);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xE9: // JP HL
                cpu->pc = register_hl(cpu);
                break;
            case 0xEA: // LD (u16), A
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->z = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    u16 v = static_cast<u16>(cpu->w) | static_cast<u16>(cpu->z) << 8;
                    memory_bus->write_u8(v, cpu->registers[Register::A]);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xEE: // XOR A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = bitwise_xor(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc));
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xEF: // RST 28h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x28;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF0: // LD A, (FF00+u8)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u8(0xFF00 + cpu->w);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF1: // POP AF
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::F] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u8(cpu->sp++);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF2: // LD A, (FF00+C)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = memory_bus->read_u8(0xFF00 + cpu->registers[Register::C]);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF3: // DI
                cpu->interrupt_master_enable = false;
                break;
            case 0xF5: // PUSH AF
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus){});
                stack_push(cpu, memory_bus, register_af(cpu));
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF6: // OR A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->registers[Register::A] = bitwise_or(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc));
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF7: // RST 30h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x30;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF8: // LD HL, SP+i8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    u16 v = add_u16_i8(cpu, cpu->sp, cpu->w);
                    cpu->w = v & 0x00FF;
                    cpu->z = (v >> 8) & 0x00FF;
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    u16 v = static_cast<u16>(cpu->w) | static_cast<u16>(cpu->z) << 8;
                    set_register_hl(cpu, v);
                });
                
                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xF9: // LD SP, HL
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->sp = register_hl(cpu);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xFA: // LD A, (u16)
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->w = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->z = memory_bus->read_u8(cpu->pc++);
                });

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    u16 v = static_cast<u16>(cpu->w) | static_cast<u16>(cpu->z) << 8;
                    cpu->registers[Register::A] = memory_bus->read_u16(v);
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xFB: // EI
                cpu->interrupt_master_enable = true;
                break;
            case 0xFE: // CP A, u8
                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    sbc(cpu, cpu->registers[Register::A], memory_bus->read_u8(cpu->pc), 0);
                    cpu->pc++;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            case 0xFF: // RST 38h
                stack_push(cpu, memory_bus, cpu->pc);

                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                {
                    cpu->pc = 0x38;
                });

                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                break;
            default:
                // TODO: Think about if these should these be changed into more segemented cases
                if (opcode < 0x40)
                {
                    u8 reg = (opcode >> 3) & 0x07;

                    switch (opcode & 0x07)
                    {
                        case 4: // INC register
                        {
                            if (reg == 6)
                            {
                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                                });

                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    memory_bus->write_u8(register_hl(cpu), inc_u8(cpu, cpu->w));
                                });
                                
                                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                            }
                            else
                            {
                                cpu->registers[reg] = inc_u8(cpu, cpu->registers[reg]);
                            }
                        } break;
                        case 5: // DEC register
                        {
                            if (reg == 6)
                            {
                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    cpu->w = memory_bus->read_u8(register_hl(cpu));
                                });

                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    memory_bus->write_u8(register_hl(cpu), dec_u8(cpu, cpu->w));
                                });

                                cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                            }
                            else
                            {
                                cpu->registers[reg] = dec_u8(cpu, cpu->registers[reg]);
                            }
                        } break;
                        case 6: // LD register, u8
                        {
                            u8 val = memory_bus->read_u8(cpu->pc++);

                            if (reg == 6)
                            {
                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    cpu->w = memory_bus->read_u8(cpu->pc++);
                                });

                                cpu->pipeline.push([](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    memory_bus->write_u8(register_hl(cpu), cpu->w);
                                });
                                
                                cpu->remaining_cycles = 12;
                            }
                            else
                            {
                                cpu->pipeline.push([reg](CPU *cpu, Memory_Bus *memory_bus)
                                {
                                    cpu->registers[reg] = memory_bus->read_u8(cpu->pc++);
                                });
                            }

                            cpu->state = CPU::STATE::EXECUTE_PIPELINE;
                        } break;
                    }
                }
        }
}

void
insert_microcode_pipeline(CPU *cpu, Memory_Bus *memory_bus, u8 opcode)
{
    if (cpu->extended)
    {
        cpu->extended = false;
        handle_extended_opcode(cpu, memory_bus, opcode);
    }
    else
    {
        handle_opcode(cpu, memory_bus, opcode);
    }
}

void
cpu_tick(CPU *cpu, Memory_Bus *memory_bus)
{
    if (cpu->halted)
    {
        return;
    }

    if (cpu->tick < 4)
    {
        cpu->tick++;
    }

    cpu->tick = 0;

    switch (cpu->state)
    {
        case CPU::STATE::READ_OPCODE:
            insert_microcode_pipeline(cpu, memory_bus, memory_bus->read_u8(cpu->pc++));
            break;
        case CPU::STATE::EXECUTE_PIPELINE:
            cpu->pipeline.front()(cpu, memory_bus);
            cpu->pipeline.pop();

            if (cpu->pipeline.empty())
            {
                cpu->state = CPU::STATE::READ_OPCODE;
            }
            break;
    }
}

bool
handle_interrupt(CPU *cpu, Memory_Bus *memory_bus, u8 interrupt_flag)
{
    u8 interrupt_enable = memory_bus->read_u8(INTERRUPT_ENABLE);

    for (u8 i = 0; i < 5; ++i)
    {
        u8 bit = 0x01 << i;
        if ((bit & interrupt_flag) && (bit & interrupt_enable))
        {
            cpu->interrupt_master_enable = false;
            interrupt_flag &= ~bit;
            memory_bus->write_u8(INTERRUPT_FLAG, interrupt_flag);

            stack_push(cpu, memory_bus, cpu->pc);

            switch (bit)
            {
                case INTERRUPT_VBLANK:
                    cpu->pc = 0x40;
                    break;
                case INTERRUPT_LCD:
                    cpu->pc = 0x48;
                    break;
                case INTERRUPT_TIMER:
                    cpu->pc = 0x50;
                    break;
                case INTERRUPT_SERIAL:
                    printf("[CPU] Serial interrupt triggered\n");
                    break;
                case INTERRUPT_JOYPAD:
                    cpu->pc = 0x60;
                    break;
            }

            cpu->remaining_cycles = 20;
            return true;
        }
    }

    return false;
}

void 
cpu_cycle(CPU *cpu, Memory_Bus *memory_bus)
{
    cpu_tick(cpu, memory_bus);

    u8 interrupt_flag = memory_bus->read_u8(INTERRUPT_FLAG);

    if (interrupt_flag)
    {
        cpu->halted = false;
    }

    if (cpu->interrupt_master_enable && interrupt_flag)
    {
        if (handle_interrupt(cpu, memory_bus, interrupt_flag))
        {
            return;
        }
    }
}

void
cpu_init(CPU *cpu, Memory_Bus *memory_bus, bool cgb, u8 old_licence_code, u8 new_license_code[2])
{
    printf("[CPU] Reset state\n");

    cpu->tick = 0;
    cpu->pc = 0x100;
    cpu->sp = 0xFFFE;

    if (cgb)
    {
        cpu->registers[Register::A] = 0x11;
        cpu->registers[Register::F] = 0x00;
        cpu->registers[Register::B] = 0x00;

        if ((old_licence_code == 0x01 || old_licence_code == 0x33) && (new_license_code[0] == 0x30 || new_license_code[1] == 0x31))
        {
            for (int i = 0; i < 16; ++i)
            {
                cpu->registers[Register::B] += memory_bus->read_u8(CARTRIDGE_TITLE + i);
            }
        }

        cpu->registers[Register::C] = 0x00;
        cpu->registers[Register::D] = 0x00;
        cpu->registers[Register::E] = 0x08;

        if (cpu->registers[Register::B] == 0x43 || cpu->registers[Register::B] == 0x58)
        {
            set_register_hl(cpu, 0x991A);
        }
        else
        {
            set_register_hl(cpu, 0x007C);
        }
    }
    else
    {
        cpu->registers[Register::A] = 0x01;
        cpu->registers[Register::F] = 0x00;
        cpu->registers[Register::B] = 0xFF;
        cpu->registers[Register::C] = 0x13;
        cpu->registers[Register::D] = 0x00;
        cpu->registers[Register::E] = 0xC1;
        cpu->registers[Register::H] = 0x84;
        cpu->registers[Register::L] = 0x03;
    }
}