#ifndef _RISCV_CSR_H_
#define _RISCV_CSR_H_


#define MSTATUS         0x300
#define MSTATUS_MIE     (1 << 3)

#define MIE             0x304
#define MIE_MSIE        (1 << 3)
#define MIE_MTIE        (1 << 7)
#define MIE_MEIE        (1 << 11)
#define MIE_MUSER       (1 << 16)

#define MIP             (0x344)
#define MIP_MSIP        (1 << 3)
#define MIP_MTIP        (1 << 7)
#define MIP_MEIP        (1 << 11)

#define MCAUSE          (0x342)
#define MCAUSE_INT      (1 << 31)
#define MCAUSE_CODE     0, 30

#define MEPC            (0x341)

#define MTVEC 0x305

#define PCER 0x7a0
#define PCCR 0x780

#define csr_set_bits(csr, value) \
  asm volatile ("csrs %[d], %[s]"\
      :                          \
      : [d] "i" (csr)            \
      , [s] "r"  (value)         \
  )

#define csr_write(csr, value)     \
  asm volatile ("csrw %[d], %[s]" \
      :                           \
      : [d] "i" (csr)             \
      , [s] "r"  (value)          \
  )

#define csr_clear_bits(csr, value) \
  asm volatile ("csrc %[d], %[s]"  \
      :                            \
      : [d] "i" (csr)              \
      , [s] "r"  (value)           \
  )

#define csr_read(csr)             \
({                                \
  unsigned int _value;            \
  asm volatile ("csrr %[d], %[s]" \
      : [d] "=r" (_value)         \
      : [s] "i"  (csr)            \
  );                              \
  _value;                         \
})

#define jtag_break()             \
({                                \
  asm volatile ("ebreak" \
  );                              \
})

#endif /* _RISCV_CSR_H_ */
