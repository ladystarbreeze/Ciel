#pragma once
#ifndef CIEL_CPU_H
#define CIEL_CPU_H


#include <memory>

enum CPU_Flags
{
    Carry = 0x1u,
    Zero  = 0x2u,
    InterruptDisable = 0x4u,
    DecimalMode = 0x8u,
    // Break  = 0x10u,
    // Unused = 0x20u,
    Overflow = 0x40u,
    Negative = 0x80u
};

struct CPU_Registers
{
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t sp;

    union
    {
        struct
        {
            uint8_t pcl;
            uint8_t pch;
        } hi_lo;

        uint16_t pc;
    } pc;
};

class MMU;

class CPU
{
private:
    CPU_Registers regs;
    std::shared_ptr<MMU> mmu;

    uint8_t i_cycle;
    uint8_t operand;
    uint16_t effective_addr;
    uint64_t cycles;

    bool page_boundary_crossed;
    bool service_nmi;

    inline void tick();
    inline void reset_ticks();
    // inline void dump_registers() const;

    [[nodiscard]] inline bool is_flag_set(CPU_Flags flag) const;
    inline void clear_flag(CPU_Flags flag);
    inline void set_flag(CPU_Flags flag);

    inline void check_nz(uint8_t value);

    [[nodiscard]] inline uint8_t read_memory(uint16_t address) const;
    inline void write_memory(uint8_t byte, uint16_t address);

    [[nodiscard]] inline uint8_t pull_stack() const;
    inline void push_stack(uint8_t byte);

    inline void oam_dma();

    inline void absolute(bool store = false);
    inline void absolute_indexed(uint8_t index, bool store = false);
    inline void immediate();
    inline void implied() const;
    inline void indexed_indirect(bool store = false);
    inline void indirect_indexed(bool store = false);
    inline void zero_page(bool store = false);
    inline void zero_page_indexed(uint8_t index, bool store = false);

    inline void add_with_carry(uint8_t value);
    inline void bit_test();
    inline void branch(bool condition);
    inline void compare(uint8_t reg);
    inline void decrement(uint8_t &reg);
    inline void increment(uint8_t &reg);
    inline void load_register(uint8_t &reg);
    inline void logical_and();
    inline void logical_or();
    inline void logical_shift_left(uint8_t &reg);
    inline void logical_shift_right(uint8_t &reg);
    inline void logical_xor();
    inline void non_maskable_interrupt();
    inline void rotate_left(uint8_t &reg);
    inline void rotate_right(uint8_t &reg);
    inline void software_interrupt();
    inline void store_register(uint8_t reg);
    inline void transfer(uint8_t source, uint8_t &target, bool txs = false);

    void adc_abs();
    void adc_abx();
    void adc_aby();
    void adc_imm();
    void adc_izx();
    void adc_izy();
    void adc_zpa();
    void adc_zpx();
    void and_abs();
    void and_abx();
    void and_aby();
    void and_imm();
    void and_izx();
    void and_izy();
    void and_zpa();
    void and_zpx();
    void asl();
    void asl_abs();
    void asl_abx();
    void asl_zpa();
    void asl_zpx();
    void bcc();
    void bcs();
    void beq();
    void bit_abs();
    void bit_zpa();
    void bmi();
    void bne();
    void bpl();
    void bvc();
    void bvs();
    void clc();
    void cld();
    void clv();
    void cmp_abs();
    void cmp_abx();
    void cmp_aby();
    void cmp_imm();
    void cmp_izx();
    void cmp_izy();
    void cmp_zpa();
    void cmp_zpx();
    void cpx_abs();
    void cpx_imm();
    void cpx_zpa();
    void cpy_abs();
    void cpy_imm();
    void cpy_zpa();
    void dec_abs();
    void dec_abx();
    void dec_zpa();
    void dec_zpx();
    void dex();
    void dey();
    void eor_abs();
    void eor_abx();
    void eor_aby();
    void eor_imm();
    void eor_izx();
    void eor_izy();
    void eor_zpa();
    void eor_zpx();
    void inc_abs();
    void inc_abx();
    void inc_zpa();
    void inc_zpx();
    void inx();
    void iny();
    void jmp_abs();
    void jmp_ind();
    void jsr();
    void lda_abs();
    void lda_abx();
    void lda_aby();
    void lda_imm();
    void lda_izx();
    void lda_izy();
    void lda_zpa();
    void lda_zpx();
    void ldx_abs();
    void ldx_aby();
    void ldx_imm();
    void ldx_zpa();
    void ldx_zpy();
    void ldy_abs();
    void ldy_abx();
    void ldy_imm();
    void ldy_zpa();
    void ldy_zpx();
    void lsr();
    void lsr_abs();
    void lsr_abx();
    void lsr_zpa();
    void lsr_zpx();
    void nop_imp();
    void ora_abs();
    void ora_abx();
    void ora_aby();
    void ora_imm();
    void ora_izx();
    void ora_izy();
    void ora_zpa();
    void ora_zpx();
    void pha();
    void php();
    void pla();
    void plp();
    void rol();
    void rol_abs();
    void rol_abx();
    void rol_zpa();
    void rol_zpx();
    void ror();
    void ror_abs();
    void ror_abx();
    void ror_zpa();
    void ror_zpx();
    void rti();
    void rts();
    void sbc_abs();
    void sbc_abx();
    void sbc_aby();
    void sbc_imm();
    void sbc_izx();
    void sbc_izy();
    void sbc_zpa();
    void sbc_zpx();
    void sec();
    void sed();
    void sei();
    void sta_abs();
    void sta_abx();
    void sta_aby();
    void sta_izx();
    void sta_izy();
    void sta_zpa();
    void sta_zpx();
    void stx_abs();
    void stx_zpa();
    void stx_zpy();
    void sty_abs();
    void sty_zpa();
    void sty_zpx();
    void tax();
    void tay();
    void tsx();
    void txa();
    void txs();
    void tya();
public:
    explicit CPU(const std::shared_ptr<MMU> &mmu);
    ~CPU();

    bool is_running;

    void run_cycle();
};


#endif //CIEL_CPU_H
