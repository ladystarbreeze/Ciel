#include "cpu.h"

#include "..//mmu/mmu.h"

CPU::CPU(const std::shared_ptr<MMU> &mmu) :
regs(), i_cycle(0), operand(0), effective_addr(0), cycles(7), page_boundary_crossed(false), service_nmi(false), is_running(true)
{
    this->mmu = mmu;

    regs.p = 0x24;
    regs.sp = 0xfd;
    regs.pc.hi_lo.pcl = read_memory(0xfffc);
    regs.pc.hi_lo.pch = read_memory(0xfffd);
}

CPU::~CPU()
= default;

void CPU::tick()
{
    ++i_cycle;
    ++cycles;
}

void CPU::reset_ticks()
{
    i_cycle = -1;
}

/* void CPU::dump_registers() const
{
    printf("    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%lu\n",
            regs.a, regs.x, regs.y, regs.p, regs.sp, cycles + 1);
} */

bool CPU::is_flag_set(const CPU_Flags flag) const
{
    return (regs.p & flag) != 0;
}

void CPU::clear_flag(const CPU_Flags flag)
{
    regs.p &= (uint8_t)(~flag);
}

void CPU::set_flag(const CPU_Flags flag)
{
    regs.p |= flag;
}

void CPU::check_nz(const uint8_t value)
{
    (value == 0) ? set_flag(Zero) : clear_flag(Zero);
    (value > 127) ? set_flag(Negative) : clear_flag(Negative);
}

uint8_t CPU::read_memory(const uint16_t address) const
{
    return mmu->read_byte(address);
}

void CPU::write_memory(const uint8_t byte, const uint16_t address)
{
    mmu->write_byte(byte, address);
}

uint8_t CPU::pull_stack() const
{
    return read_memory(0x100u | regs.sp);
}

void CPU::push_stack(const uint8_t byte)
{
    write_memory(byte, 0x100u | regs.sp--);
}

void CPU::oam_dma()
{
    static uint8_t byte;
    static uint8_t lo = 0;
    static uint16_t elapsed = 0;

    switch (elapsed % 2)
    {
        case 0:
            byte = read_memory((uint16_t)(mmu->oam_hi << 8u) | lo);
            ++lo;
            break;
        case 1:
            write_memory(byte, 0x2004);
            break;
    }

    ++elapsed;

    if (elapsed == 513)
    {
        mmu->oam_dma = false;
        lo = 0;
        elapsed = 0;

        // printf("[2A03] OAM-DMA finished\n");
    }
}

void CPU::absolute(const bool store)
{
    static uint8_t hi;
    static uint8_t lo;

    switch (i_cycle)
    {
        case 1:
            lo = read_memory(regs.pc.pc++);
            break;
        case 2:
            hi = read_memory(regs.pc.pc++);
            effective_addr = (uint16_t)(hi << 8u) | lo;
            break;
        case 3:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }
            break;
    }
}

void CPU::absolute_indexed(const uint8_t index, const bool store)
{
    static uint8_t hi;
    static uint8_t lo;
    static uint16_t correct_addr;

    switch (i_cycle)
    {
        case 1:
            page_boundary_crossed = false;
            lo = read_memory(regs.pc.pc++);
            break;
        case 2:
            hi = read_memory(regs.pc.pc++);
            correct_addr = ((uint16_t)(hi << 8u) | lo) + index;
            lo += index;
            effective_addr = (uint16_t)(hi << 8u) | lo;
            break;
        case 3:
            operand = read_memory(effective_addr);

            if (effective_addr != correct_addr)
            {
                page_boundary_crossed = true;
                effective_addr += 0x100u;
            }
            break;
        case 4:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }

            page_boundary_crossed = false;
            break;
    }
}

void CPU::immediate()
{
    operand = read_memory(regs.pc.pc++);
}

void CPU::implied() const
{
    static uint8_t dummy;

    dummy = read_memory(regs.pc.pc);
}

void CPU::indexed_indirect(const bool store)
{
    static uint8_t dummy;
    static uint8_t hi;
    static uint8_t lo;
    static uint8_t pointer;

    switch (i_cycle)
    {
        case 1:
            pointer = read_memory(regs.pc.pc++);
            break;
        case 2:
            dummy = read_memory(pointer);
            pointer += regs.x;
            break;
        case 3:
            lo = read_memory(pointer++);
            break;
        case 4:
            hi = read_memory(pointer);
            effective_addr = (uint16_t)(hi << 8u) | lo;
            break;
        case 5:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }
            break;
    }
}

void CPU::indirect_indexed(const bool store)
{
    static uint8_t hi;
    static uint8_t lo;
    static uint8_t pointer;
    static uint16_t correct_addr;

    switch (i_cycle)
    {
        case 1:
            page_boundary_crossed = false;
            pointer = read_memory(regs.pc.pc++);
            break;
        case 2:
            lo = read_memory(pointer++);
            break;
        case 3:
            hi = read_memory(pointer);
            correct_addr = ((uint16_t)(hi << 8u) | lo) + regs.y;
            lo += regs.y;
            effective_addr = (uint16_t)(hi << 8u) | lo;
            break;
        case 4:
            operand = read_memory(effective_addr);

            if (effective_addr != correct_addr)
            {
                page_boundary_crossed = true;
                effective_addr += 0x100u;
            }
            break;
        case 5:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }

            page_boundary_crossed = false;
            break;

    }
}

void CPU::zero_page(const bool store)
{
    switch (i_cycle)
    {
        case 1:
            effective_addr = read_memory(regs.pc.pc++);
            break;
        case 2:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }
            break;
    }
}

void CPU::zero_page_indexed(const uint8_t index, const bool store)
{
    static uint8_t dummy;
    static uint8_t zero_page_address;

    switch (i_cycle)
    {
        case 1:
            zero_page_address = read_memory(regs.pc.pc++);
            break;
        case 2:
            dummy = read_memory(zero_page_address);
            zero_page_address += index;
            effective_addr = zero_page_address;
            break;
        case 3:
            if (!store)
            {
                operand = read_memory(effective_addr);
            }
            break;
    }
}

void CPU::add_with_carry(const uint8_t value)
{
    uint16_t result = regs.a + value + (regs.p & 0x1u);

    (result > 255) ? set_flag(Carry) : clear_flag(Carry);
    check_nz((uint8_t)result);
    (((regs.a & 0x80u) == (value & 0x80u)) && ((regs.a & 0x80u) != (result & 0x80u))) ?
    set_flag(Overflow) : clear_flag(Overflow);

    regs.a = (uint8_t)result;
}

void CPU::bit_test()
{
    uint8_t result = regs.a & operand;

    (result == 0) ? set_flag(Zero) : clear_flag(Zero);
    ((operand & 0x40u) != 0) ? set_flag(Overflow) : clear_flag(Overflow);
    ((operand & 0x80u) != 0) ? set_flag(Negative) : clear_flag(Negative);
}

void CPU::branch(const bool condition)
{
    static int8_t relative_offset;
    static uint16_t correct_addr;

    switch (i_cycle)
    {
        case 1:
            relative_offset = (int8_t)read_memory(regs.pc.pc++);

            if (!condition)
            {
                reset_ticks();
            }
            break;
        case 2:
            correct_addr = regs.pc.pc + relative_offset;
            regs.pc.hi_lo.pcl += relative_offset;

            if (regs.pc.pc == correct_addr)
            {
                reset_ticks();
            }
            break;
        case 3:
            (regs.pc.pc > correct_addr) ? --regs.pc.hi_lo.pch : ++regs.pc.hi_lo.pch;

            reset_ticks();
            break;
    }
}

void CPU::compare(const uint8_t reg)
{
    uint8_t result = reg - operand;

    (reg >= operand) ? set_flag(Carry) : clear_flag(Carry);
    (result == 0) ? set_flag(Zero) : clear_flag(Zero);
    ((result & 0x80u) != 0) ? set_flag(Negative) : clear_flag(Negative);
}

void CPU::decrement(uint8_t &reg)
{
    --reg;

    check_nz(reg);
}

void CPU::increment(uint8_t &reg)
{
    ++reg;

    check_nz(reg);
}

void CPU::load_register(uint8_t &reg)
{
    reg = operand;

    check_nz(reg);
}

void CPU::logical_and()
{
    regs.a &= operand;

    check_nz(regs.a);
}

void CPU::logical_or()
{
    regs.a |= operand;

    check_nz(regs.a);
}

void CPU::logical_shift_left(uint8_t &reg)
{
    bool carry = (reg & 0x80u) != 0;

    reg <<= 1u;
    regs.p = (regs.p & 0xfeu) | carry;

    check_nz(reg);
}

void CPU::logical_shift_right(uint8_t &reg)
{
    bool carry = (reg & 0x1u) != 0;

    reg >>= 1u;
    regs.p = (regs.p & 0xfeu) | carry;

    (reg == 0) ? set_flag(Zero) : clear_flag(Zero);
    clear_flag(Negative);
}

void CPU::logical_xor()
{
    regs.a ^= operand;

    check_nz(regs.a);
}

void CPU::non_maskable_interrupt()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            --regs.pc.pc;
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            push_stack(regs.pc.hi_lo.pch);
            break;
        case 3:
            push_stack(regs.pc.hi_lo.pcl);
            break;
        case 4:
            push_stack(regs.p);
            break;
        case 5:
            regs.pc.hi_lo.pcl = read_memory(0xfffa);
            break;
        case 6:
            regs.pc.hi_lo.pch = read_memory(0xfffb);
            service_nmi = false;

            reset_ticks();
            break;
    }
}

void CPU::rotate_left(uint8_t &reg)
{
    bool carry = (reg & 0x80u) != 0;

    reg <<= 1u;
    reg |= (regs.p & 0x1u);
    regs.p = (regs.p & 0xfeu) | carry;

    check_nz(reg);
}

void CPU::rotate_right(uint8_t &reg)
{
    bool carry = (reg & 0x1u) != 0;

    reg >>= 1u;
    reg |= ((regs.p & 0x1u) << 7u);
    regs.p = (regs.p & 0xfeu) | carry;

    check_nz(reg);
}

void CPU::software_interrupt()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc++);
            break;
        case 2:
            push_stack(regs.pc.hi_lo.pch);
            break;
        case 3:
            push_stack(regs.pc.hi_lo.pcl);
            break;
        case 4:
            push_stack(regs.p | 0x10u);
            break;
        case 5:
            regs.pc.hi_lo.pcl = read_memory(0xfffe);
            break;
        case 6:
            regs.pc.hi_lo.pch = read_memory(0xffff);
            reset_ticks();
            break;
    }
}

void CPU::store_register(const uint8_t reg)
{
    write_memory(reg, effective_addr);
}

void CPU::transfer(const uint8_t source, uint8_t &target, const bool txs)
{
    implied();

    target = source;

    if (!txs)
    {
        check_nz(target);
    }

    reset_ticks();
}

void CPU::adc_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_imm()
{
    immediate();
    add_with_carry(operand);
    reset_ticks();
}

void CPU::adc_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::adc_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        add_with_carry(operand);
        reset_ticks();
    }
}

void CPU::and_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_imm()
{
    immediate();
    logical_and();
    reset_ticks();
}

void CPU::and_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::and_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        logical_and();
        reset_ticks();
    }
}

void CPU::asl()
{
    implied();
    logical_shift_left(regs.a);
    reset_ticks();
}

void CPU::asl_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            logical_shift_left(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::asl_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            logical_shift_left(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::asl_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            logical_shift_left(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::asl_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            logical_shift_left(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::bcc()
{
    branch(!is_flag_set(Carry));
}

void CPU::bcs()
{
    branch(is_flag_set(Carry));
}

void CPU::beq()
{
    branch(is_flag_set(Zero));
}

void CPU::bit_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        bit_test();
        reset_ticks();
    }
}

void CPU::bit_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        bit_test();
        reset_ticks();
    }
}

void CPU::bmi()
{
    branch(is_flag_set(Negative));
}

void CPU::bne()
{
    branch(!is_flag_set(Zero));
}

void CPU::bpl()
{
    branch(!is_flag_set(Negative));
}

void CPU::bvc()
{
    branch(!is_flag_set(Overflow));
}

void CPU::bvs()
{
    branch(is_flag_set(Overflow));
}

void CPU::clc()
{
    implied();
    clear_flag(Carry);
    reset_ticks();
}

void CPU::cld()
{
    implied();
    clear_flag(DecimalMode);
    reset_ticks();
}

void CPU::clv()
{
    implied();
    clear_flag(Overflow);
    reset_ticks();
}

void CPU::cmp_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_imm()
{
    immediate();
    compare(regs.a);
    reset_ticks();
}

void CPU::cmp_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cmp_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        compare(regs.a);
        reset_ticks();
    }
}

void CPU::cpx_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        compare(regs.x);
        reset_ticks();
    }
}

void CPU::cpx_imm()
{
    immediate();
    compare(regs.x);
    reset_ticks();
}

void CPU::cpx_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        compare(regs.x);
        reset_ticks();
    }
}

void CPU::cpy_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        compare(regs.y);
        reset_ticks();
    }
}

void CPU::cpy_imm()
{
    immediate();
    compare(regs.y);
    reset_ticks();
}

void CPU::cpy_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        compare(regs.y);
        reset_ticks();
    }
}

void CPU::dec_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            decrement(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::dec_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            decrement(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::dec_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            decrement(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::dec_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            decrement(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::dex()
{
    implied();
    decrement(regs.x);
    reset_ticks();
}

void CPU::dey()
{
    implied();
    decrement(regs.y);
    reset_ticks();
}

void CPU::eor_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_imm()
{
    immediate();
    logical_xor();
    reset_ticks();
}

void CPU::eor_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::eor_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        logical_xor();
        reset_ticks();
    }
}

void CPU::inc_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            increment(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::inc_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            increment(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::inc_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            increment(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::inc_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            increment(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::inx()
{
    implied();
    increment(regs.x);
    reset_ticks();
}

void CPU::iny()
{
    implied();
    increment(regs.y);
    reset_ticks();
}

void CPU::jmp_abs()
{
    static uint8_t pcl;

    switch (i_cycle)
    {
        case 1:
            pcl = read_memory(regs.pc.pc++);
            break;
        case 2:
            regs.pc.hi_lo.pch = read_memory(regs.pc.pc);
            regs.pc.hi_lo.pcl = pcl;

            reset_ticks();
            break;
    }
}

void CPU::jmp_ind()
{
    static uint8_t pointer_hi;
    static uint8_t pointer_lo;

    switch (i_cycle)
    {
        case 1:
            pointer_lo = read_memory(regs.pc.pc++);
            break;
        case 2:
            pointer_hi = read_memory(regs.pc.pc++);
            break;
        case 3:
            regs.pc.hi_lo.pcl = read_memory((uint16_t)(pointer_hi << 8u) | pointer_lo);
            ++pointer_lo;
            break;
        case 4:
            regs.pc.hi_lo.pch = read_memory((uint16_t)(pointer_hi << 8u) | pointer_lo);

            reset_ticks();
            break;
    }
}

void CPU::jsr()
{
    static uint8_t pcl;

    switch (i_cycle)
    {
        case 1:
            pcl = read_memory(regs.pc.pc++);
            break;
        case 2:
            // internal operation?
            break;
        case 3:
            push_stack(regs.pc.hi_lo.pch);
            break;
        case 4:
            push_stack(regs.pc.hi_lo.pcl);
            break;
        case 5:
            regs.pc.hi_lo.pch = read_memory(regs.pc.pc);
            regs.pc.hi_lo.pcl = pcl;

            reset_ticks();
            break;
    }
}

void CPU::lda_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_imm()
{
    immediate();

    load_register(regs.a);
    reset_ticks();
}

void CPU::lda_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::lda_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        load_register(regs.a);
        reset_ticks();
    }
}

void CPU::ldx_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        load_register(regs.x);
        reset_ticks();
    }
}

void CPU::ldx_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        load_register(regs.x);
        reset_ticks();
    }
}

void CPU::ldx_imm()
{
    immediate();

    load_register(regs.x);
    reset_ticks();
}

void CPU::ldx_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        load_register(regs.x);
        reset_ticks();
    }
}

void CPU::ldx_zpy()
{
    zero_page_indexed(regs.y);

    if (i_cycle == 3)
    {
        load_register(regs.x);
        reset_ticks();
    }
}

void CPU::ldy_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        load_register(regs.y);
        reset_ticks();
    }
}

void CPU::ldy_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        load_register(regs.y);
        reset_ticks();
    }
}

void CPU::ldy_imm()
{
    immediate();

    load_register(regs.y);
    reset_ticks();
}

void CPU::ldy_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        load_register(regs.y);
        reset_ticks();
    }
}

void CPU::ldy_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        load_register(regs.y);
        reset_ticks();
    }
}

void CPU::lsr()
{
    implied();
    logical_shift_right(regs.a);
    reset_ticks();
}

void CPU::lsr_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            logical_shift_right(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::lsr_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            logical_shift_right(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::lsr_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            logical_shift_right(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::lsr_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            logical_shift_right(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::nop_imp()
{
    implied();
    reset_ticks();
}

void CPU::ora_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_imm()
{
    immediate();
    logical_or();
    reset_ticks();
}

void CPU::ora_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::ora_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        logical_or();
        reset_ticks();
    }
}

void CPU::pha()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            push_stack(regs.a);
            reset_ticks();
            break;
    }
}

void CPU::php()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            push_stack(regs.p | 0x30u);
            reset_ticks();
            break;
    }
}

void CPU::pla()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            ++regs.sp;
            break;
        case 3:
            regs.a = pull_stack();

            check_nz(regs.a);
            reset_ticks();
            break;
    }
}

void CPU::plp()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            ++regs.sp;
            break;
        case 3:
            regs.p = (regs.p & 0x30u) | (uint8_t)(pull_stack() & (uint8_t)(~0x30u));

            reset_ticks();
            break;
    }
}

void CPU::rol()
{
    implied();
    rotate_left(regs.a);
    reset_ticks();
}

void CPU::rol_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            rotate_left(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::rol_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            rotate_left(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::rol_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            rotate_left(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::rol_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            rotate_left(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::ror()
{
    implied();
    rotate_right(regs.a);
    reset_ticks();
}

void CPU::ror_abs()
{
    absolute();

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            rotate_right(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::ror_abx()
{
    absolute_indexed(regs.x);

    switch (i_cycle)
    {
        case 5:
            write_memory(operand, effective_addr);
            rotate_right(operand);
            break;
        case 6:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::ror_zpa()
{
    zero_page();

    switch (i_cycle)
    {
        case 3:
            write_memory(operand, effective_addr);
            rotate_right(operand);
            break;
        case 4:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::ror_zpx()
{
    zero_page_indexed(regs.x);

    switch (i_cycle)
    {
        case 4:
            write_memory(operand, effective_addr);
            rotate_right(operand);
            break;
        case 5:
            write_memory(operand, effective_addr);
            reset_ticks();
            break;
    }
}

void CPU::rti()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            ++regs.sp;
            break;
        case 3:
            regs.p = (regs.p & 0x30u) | (uint8_t)(pull_stack() & (uint8_t)(~0x30u));
            ++regs.sp;
            break;
        case 4:
            regs.pc.hi_lo.pcl = pull_stack();
            ++regs.sp;
            break;
        case 5:
            regs.pc.hi_lo.pch = pull_stack();

            reset_ticks();
            break;
    }
}

void CPU::rts()
{
    static uint8_t dummy;

    switch (i_cycle)
    {
        case 1:
            dummy = read_memory(regs.pc.pc);
            break;
        case 2:
            ++regs.sp;
            break;
        case 3:
            regs.pc.hi_lo.pcl = pull_stack();
            ++regs.sp;
            break;
        case 4:
            regs.pc.hi_lo.pch = pull_stack();
            break;
        case 5:
            ++regs.pc.pc;

            reset_ticks();
            break;
    }
}

void CPU::sbc_abs()
{
    absolute();

    if (i_cycle == 3)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_abx()
{
    absolute_indexed(regs.x);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_aby()
{
    absolute_indexed(regs.y);

    if (i_cycle >= 3 && !page_boundary_crossed)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_imm()
{
    immediate();
    add_with_carry(~operand);
    reset_ticks();
}

void CPU::sbc_izx()
{
    indexed_indirect();

    if (i_cycle == 5)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_izy()
{
    indirect_indexed();

    if (i_cycle >= 4 && !page_boundary_crossed)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_zpa()
{
    zero_page();

    if (i_cycle == 2)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sbc_zpx()
{
    zero_page_indexed(regs.x);

    if (i_cycle == 3)
    {
        add_with_carry(~operand);
        reset_ticks();
    }
}

void CPU::sec()
{
    implied();
    set_flag(Carry);
    reset_ticks();
}

void CPU::sed()
{
    implied();
    set_flag(DecimalMode);
    reset_ticks();
}

void CPU::sei()
{
    implied();
    set_flag(InterruptDisable);
    reset_ticks();
}

void CPU::sta_abs()
{
    absolute(true);

    if (i_cycle == 3)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_abx()
{
    absolute_indexed(regs.x, true);

    if (i_cycle == 4)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_aby()
{
    absolute_indexed(regs.y, true);

    if (i_cycle == 4)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_izx()
{
    indexed_indirect(true);

    if (i_cycle == 5)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_izy()
{
    indirect_indexed(true);

    if (i_cycle == 5)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_zpa()
{
    zero_page(true);

    if (i_cycle == 2)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::sta_zpx()
{
    zero_page_indexed(regs.x, true);

    if (i_cycle == 3)
    {
        store_register(regs.a);
        reset_ticks();
    }
}

void CPU::stx_abs()
{
    absolute(true);

    if (i_cycle == 3)
    {
        store_register(regs.x);
        reset_ticks();
    }
}

void CPU::stx_zpa()
{
    zero_page(true);

    if (i_cycle == 2)
    {
        store_register(regs.x);
        reset_ticks();
    }
}

void CPU::stx_zpy()
{
    zero_page_indexed(regs.y, true);

    if (i_cycle == 3)
    {
        store_register(regs.x);
        reset_ticks();
    }
}

void CPU::sty_abs()
{
    absolute(true);

    if (i_cycle == 3)
    {
        store_register(regs.y);
        reset_ticks();
    }
}

void CPU::sty_zpa()
{
    zero_page(true);

    if (i_cycle == 2)
    {
        store_register(regs.y);
        reset_ticks();
    }
}

void CPU::sty_zpx()
{
    zero_page_indexed(regs.x, true);

    if (i_cycle == 3)
    {
        store_register(regs.y);
        reset_ticks();
    }
}

void CPU::tax()
{
    transfer(regs.a, regs.x);
}

void CPU::tay()
{
    transfer(regs.a, regs.y);
}

void CPU::tsx()
{
    transfer(regs.sp, regs.x);
}

void CPU::txa()
{
    transfer(regs.x, regs.a);
}

void CPU::txs()
{
    transfer(regs.x, regs.sp, true);
}

void CPU::tya()
{
    transfer(regs.y, regs.a);
}

void CPU::run_cycle()
{
    static uint8_t opcode;

    if (mmu->oam_dma)
    {
        // printf("[2A03] OAM-DMA\n");
        oam_dma();
    }

    if (i_cycle == 0)
    {
        opcode = read_memory(regs.pc.pc++);

        if (mmu->nmi_pending)
        {
            // printf("[2A03] NMI acknowledged!\n");

            service_nmi = true;
            mmu->nmi_pending = false;
        }

        tick();
        return;
    }

    if (service_nmi)
    {
        non_maskable_interrupt();
        tick();
        return;
    }

    switch (opcode)
    {
        case 0x00:
            software_interrupt();
            break;
        case 0x01:
            ora_izx();
            break;
        case 0x05:
            ora_zpa();
            break;
        case 0x06:
            asl_zpa();
            break;
        case 0x08:
            php();
            break;
        case 0x09:
            ora_imm();
            break;
        case 0x0a:
            asl();
            break;
        case 0x0d:
            ora_abs();
            break;
        case 0x0e:
            asl_abs();
            break;
        case 0x10:
            bpl();
            break;
        case 0x11:
            ora_izy();
            break;
        case 0x15:
            ora_zpx();
            break;
        case 0x16:
            asl_zpx();
            break;
        case 0x18:
            clc();
            break;
        case 0x19:
            ora_aby();
            break;
        case 0x1d:
            ora_abx();
            break;
        case 0x1e:
            asl_abx();
            break;
        case 0x20:
            jsr();
            break;
        case 0x21:
            and_izx();
            break;
        case 0x24:
            bit_zpa();
            break;
        case 0x25:
            and_zpa();
            break;
        case 0x26:
            rol_zpa();
            break;
        case 0x28:
            plp();
            break;
        case 0x29:
            and_imm();
            break;
        case 0x2a:
            rol();
            break;
        case 0x2c:
            bit_abs();
            break;
        case 0x2d:
            and_abs();
            break;
        case 0x2e:
            rol_abs();
            break;
        case 0x30:
            bmi();
            break;
        case 0x31:
            and_izy();
            break;
        case 0x35:
            and_zpx();
            break;
        case 0x36:
            rol_zpx();
            break;
        case 0x38:
            sec();
            break;
        case 0x39:
            and_aby();
            break;
        case 0x3d:
            and_abx();
            break;
        case 0x3e:
            rol_abx();
            break;
        case 0x40:
            rti();
            break;
        case 0x41:
            eor_izx();
            break;
        case 0x45:
            eor_zpa();
            break;
        case 0x46:
            lsr_zpa();
            break;
        case 0x48:
            pha();
            break;
        case 0x49:
            eor_imm();
            break;
        case 0x4a:
            lsr();
            break;
        case 0x4c:
            jmp_abs();
            break;
        case 0x4d:
            eor_abs();
            break;
        case 0x4e:
            lsr_abs();
            break;
        case 0x50:
            bvc();
            break;
        case 0x51:
            eor_izy();
            break;
        case 0x56:
            lsr_zpx();
            break;
        case 0x55:
            eor_zpx();
            break;
        case 0x59:
            eor_aby();
            break;
        case 0x5d:
            eor_abx();
            break;
        case 0x5e:
            lsr_abx();
            break;
        case 0x60:
            rts();
            break;
        case 0x61:
            adc_izx();
            break;
        case 0x65:
            adc_zpa();
            break;
        case 0x66:
            ror_zpa();
            break;
        case 0x68:
            pla();
            break;
        case 0x69:
            adc_imm();
            break;
        case 0x6a:
            ror();
            break;
        case 0x6c:
            jmp_ind();
            break;
        case 0x6d:
            adc_abs();
            break;
        case 0x6e:
            ror_abs();
            break;
        case 0x70:
            bvs();
            break;
        case 0x71:
            adc_izy();
            break;
        case 0x75:
            adc_zpx();
            break;
        case 0x76:
            ror_zpx();
            break;
        case 0x78:
            sei();
            break;
        case 0x79:
            adc_aby();
            break;
        case 0x7d:
            adc_abx();
            break;
        case 0x7e:
            ror_abx();
            break;
        case 0x81:
            sta_izx();
            break;
        case 0x84:
            sty_zpa();
            break;
        case 0x85:
            sta_zpa();
            break;
        case 0x86:
            stx_zpa();
            break;
        case 0x88:
            dey();
            break;
        case 0x8a:
            txa();
            break;
        case 0x8c:
            sty_abs();
            break;
        case 0x8d:
            sta_abs();
            break;
        case 0x8e:
            stx_abs();
            break;
        case 0x90:
            bcc();
            break;
        case 0x91:
            sta_izy();
            break;
        case 0x94:
            sty_zpx();
            break;
        case 0x95:
            sta_zpx();
            break;
        case 0x96:
            stx_zpy();
            break;
        case 0x98:
            tya();
            break;
        case 0x99:
            sta_aby();
            break;
        case 0x9a:
            txs();
            break;
        case 0x9d:
            sta_abx();
            break;
        case 0xa0:
            ldy_imm();
            break;
        case 0xa1:
            lda_izx();
            break;
        case 0xa2:
            ldx_imm();
            break;
        case 0xa4:
            ldy_zpa();
            break;
        case 0xa5:
            lda_zpa();
            break;
        case 0xa6:
            ldx_zpa();
            break;
        case 0xa8:
            tay();
            break;
        case 0xa9:
            lda_imm();
            break;
        case 0xaa:
            tax();
            break;
        case 0xac:
            ldy_abs();
            break;
        case 0xad:
            lda_abs();
            break;
        case 0xae:
            ldx_abs();
            break;
        case 0xb0:
            bcs();
            break;
        case 0xb1:
            lda_izy();
            break;
        case 0xb4:
            ldy_zpx();
            break;
        case 0xb5:
            lda_zpx();
            break;
        case 0xb6:
            ldx_zpy();
            break;
        case 0xb8:
            clv();
            break;
        case 0xb9:
            lda_aby();
            break;
        case 0xba:
            tsx();
            break;
        case 0xbc:
            ldy_abx();
            break;
        case 0xbd:
            lda_abx();
            break;
        case 0xbe:
            ldx_aby();
            break;
        case 0xc0:
            cpy_imm();
            break;
        case 0xc1:
            cmp_izx();
            break;
        case 0xc4:
            cpy_zpa();
            break;
        case 0xc5:
            cmp_zpa();
            break;
        case 0xc6:
            dec_zpa();
            break;
        case 0xc8:
            iny();
            break;
        case 0xc9:
            cmp_imm();
            break;
        case 0xca:
            dex();
            break;
        case 0xcc:
            cpy_abs();
            break;
        case 0xcd:
            cmp_abs();
            break;
        case 0xce:
            dec_abs();
            break;
        case 0xd0:
            bne();
            break;
        case 0xd1:
            cmp_izy();
            break;
        case 0xd5:
            cmp_zpx();
            break;
        case 0xd6:
            dec_zpx();
            break;
        case 0xd8:
            cld();
            break;
        case 0xd9:
            cmp_aby();
            break;
        case 0xdd:
            cmp_abx();
            break;
        case 0xde:
            dec_abx();
            break;
        case 0xe0:
            cpx_imm();
            break;
        case 0xe1:
            sbc_izx();
            break;
        case 0xe4:
            cpx_zpa();
            break;
        case 0xe5:
            sbc_zpa();
            break;
        case 0xe6:
            inc_zpa();
            break;
        case 0xe8:
            inx();
            break;
        case 0xe9:
            sbc_imm();
            break;
        case 0xea:
            nop_imp();
            break;
        case 0xec:
            cpx_abs();
            break;
        case 0xed:
            sbc_abs();
            break;
        case 0xee:
            inc_abs();
            break;
        case 0xf0:
            beq();
            break;
        case 0xf1:
            sbc_izy();
            break;
        case 0xf5:
            sbc_zpx();
            break;
        case 0xf6:
            inc_zpx();
            break;
        case 0xf8:
            sed();
            break;
        case 0xf9:
            sbc_aby();
            break;
        case 0xfd:
            sbc_abx();
            break;
        case 0xfe:
            inc_abx();
            break;
        default:
            printf("[2A03] Opcode: %02X", opcode);
            throw std::runtime_error("[2A03] Unknown opcode!");
    }

    tick();
}