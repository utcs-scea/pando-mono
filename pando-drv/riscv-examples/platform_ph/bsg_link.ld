/*********************************************************
 BSG Manycore Linker Script

 data default: dmem
 dram memory size: 0x80000000
 imem allocated size: 0x01000000
 stack pointer init: 0x00001000

 Generated at 2023-09-22 09:36:18.765876
**********************************************************/

MEMORY {
DMEM_VMA (rw)   : ORIGIN = 0x0000000000000000, LENGTH = 0x00020000
DRAM_T_LMA (rx) : ORIGIN = 0x0000000002000000, LENGTH = 0x00100000
DRAM_D_LMA (rw) : ORIGIN = 0x0000000002100000, LENGTH = 0x00f00000
}

SECTIONS {

.dmem  :
AT(0x0) {
  *(.dmem.interrupt)
  *(.dmem)
  *(.dmem.*)
  . = ALIGN(16);

} >DMEM_VMA


.tdata.dmem  :
AT(LOADADDR(.dmem) + ADDR(.tdata.dmem) - ADDR(.dmem)) {
  *(.tdata)
  *(.tdata*)
  . = ALIGN(16);

} >DMEM_VMA

.tbss.dmem  :
AT(LOADADDR(.tdata.dmem) + ADDR(.tbss.dmem) - ADDR(.tdata.dmem)) {
  *(.tbss)
  *(.tbss*)
  . = ALIGN(16);

} >DMEM_VMA

.striped.data.dmem  :
AT(LOADADDR(.tbss.dmem) + ADDR(.striped.data.dmem) - ADDR(.tbss.dmem)) {
  *(.striped.data)
  . = ALIGN(16);

} >DMEM_VMA

__dmem_end = .;
ASSERT((__dmem_end <= 0x00020000), "Error: dmem size limit exceeded")

.text.dram 0x0 :
 {
  *(.text.interrupt)
  *(.crtbegin)
  *(.text)
  *(.text.startup)
  *(.text.*)
  PROVIDE (__.text.dram_end = .);
  ASSERT((__.text.dram_end <= 0x00100000), "Error: .text.dram section exceeded limit 0x01000000");
  . = ALIGN(16);

}  AT>DRAM_T_LMA
. = 0x0000000002100000;
__global_pointer$ = .;
.eh_frame.dram  :
 {
  *(.eh_frame)
  *(.eh_frame*)
  . = ALIGN(16);

}  AT>DRAM_D_LMA

.rodata.dram  :
 {
  *(.rodata)
  *(.rodata*)
  *(.srodata.cst16)
  *(.srodata.cst8)
  *(.srodata.cst4)
  *(.srodata.cst2)
  *(.srodata*)
  . = ALIGN(16);

}  AT>DRAM_D_LMA

.data.dram  :
 {
  *(.dram)
  *(.dram.*)
  *(.data)
  *(.data*)
  . = ALIGN(16);
 }  AT>DRAM_D_LMA

.sdata.dram  :
 {
  *(.sdata)
  *(.sdata.*)
  *(.sdata*)
  *(.sdata*.*.gnu.linkonce.s.*)
  *(.sbss)
  *(.sbss.*)
  *(.gnu.linkonce.sb.*)
  *(.scommon)
  . = ALIGN(16);
 }  AT>DRAM_D_LMA

.bss.dram  :
{
  *(.bss)
  *(.bss*)
  . = ALIGN(16);
}  AT>DRAM_D_LMA

_gp = ADDR(.sdata.dram);
_sp = 0x00001000;
_bsg_data_start_addr = 0x00000000;
_bsg_data_end_addr = ADDR(.striped.data.dmem) + SIZEOF(.striped.data.dmem);
_bsg_striped_data_start = ADDR(.striped.data.dmem);
_bsg_dram_end_addr = LOADADDR(.bss.dram) + SIZEOF(.bss.dram);
_edata = LOADADDR(.bss.dram);
_end = _bsg_dram_end_addr;
end = _end;
ENTRY(_start)
}
