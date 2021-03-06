/*SEARCH_DIR("C:\Program Files\Raisonance\Ride\Lib\ARM")
EXTERN( __io_putchar )
EXTERN( _write )
GROUP(
"std_sbrk_thumb.lib"
"smallprintf_thumb.a"
"libc.a"
"libm.a"
)
*/
/* default stack sizes. 

These are used by the startup in order to allocate stacks for the different modes.
*/

__Stack_Size = 0x400 ;

PROVIDE ( _Stack_Size = __Stack_Size ) ;

__Stack_Init = _estack  - __Stack_Size ;

/*"PROVIDE" allows to easily override these values from an object file or the commmand line.*/
PROVIDE ( _Stack_Init = __Stack_Init ) ;

/*
There will be a link error if there is not this amount of RAM free at the end.
*/
_Minimum_Stack_Size = 0x400 ;



/*
this sends all unreferenced IRQHandlers to reset
*/

PROVIDE(Default_Handler = 0 );
PROVIDE(NMI_Handler = Default_Handler );
PROVIDE(HardFault_Handler = Default_Handler );
PROVIDE(MemManage_Handler = Default_Handler );
PROVIDE(BusFault_Handler = Default_Handler );
PROVIDE(UsageFault_Handler = Default_Handler );
PROVIDE(SVC_Handler = Default_Handler );
PROVIDE(DebugMonitor_Handler = Default_Handler );
PROVIDE(PendSV_Handler = Default_Handler );
PROVIDE(SysTick_Handler = Default_Handler );
PROVIDE(halTimer1Isr = Default_Handler );
PROVIDE(halTimer2Isr = Default_Handler );
PROVIDE(halManagementIsr = Default_Handler );
PROVIDE(halBaseBandIsr = Default_Handler );
PROVIDE(halSleepTimerIsr = Default_Handler );
PROVIDE(halSc1Isr = Default_Handler );
PROVIDE(halSc2Isr = Default_Handler );
PROVIDE(halSecurityIsr = Default_Handler );
PROVIDE(halStackMacTimerIsr = Default_Handler );
PROVIDE(stmRadioTransmitIsr = Default_Handler );
PROVIDE(stmRadioReceiveIsr = Default_Handler );
PROVIDE(halAdcIsr = Default_Handler );
PROVIDE(halIrqAIsr = Default_Handler );
PROVIDE(halIrqBIsr = Default_Handler );
PROVIDE(halIrqCIsr = Default_Handler );
PROVIDE(halIrqDIsr = Default_Handler );
PROVIDE(halDebugIsr = Default_Handler );
/*PROVIDE(stSerialPrintf = printf );*/

/******************************************************************************/
/*                       Peripheral memory map                                */
/******************************************************************************/
/*this allows to compile the ST lib in "non-debug" mode*/


/* include the memory spaces definitions sub-script */
MEMORY
{
  RAM_region (xrw) :            ORIGIN = 0x20000000,            LENGTH = 8K
  ROM_region (rx) :           ORIGIN = 0x08000000,            LENGTH = 128K-3K
  NVM_region (rx) :           ORIGIN = 0x0801F400,            LENGTH = 3K
  FIB_region (ra) :            ORIGIN = 0x08040000,            LENGTH = 2K
}

/* higher address of the user mode stack */
_estack = 0x20002000;

/* Sections management for FLASH mode */

/* Sections Definitions */

SECTIONS
{

    /* for Cortex devices, the beginning of the startup code is stored in the .isr_vector section, which goes to FLASH */
    .isr_vector :
    {
	. = ALIGN(4);
        KEEP(*(.isr_vector))            /* Startup code */
	. = ALIGN(4);
    } >ROM_region
 
    /* for some STRx devices, the beginning of the startup code is stored in the .flashtext section, which goes to FLASH */
    .flashtext :
    {
	. = ALIGN(4);
        *(.flashtext)            /* Startup code */
	. = ALIGN(4);
    } >ROM_region
 
    /* the program code is stored in the .text section, which goes to Flash */
    .text :
    {
	    . = ALIGN(4);
	    
        *(.text)                   /* remaining code */
        *(.text.*)                   /* remaining code */
        *(.rodata)                 /* read-only data (constants) */
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)

	    . = ALIGN(4);
   	 _etext = .;
	    /* This is used by the startup in order to initialize the .data secion */
   	 _sidata = _etext;
    } >ROM_region
    
    NVM (NOLOAD):
    {
      . = ALIGN(1024);
      KEEP(*(NVM))
      . = ALIGN(4);
    } > NVM_region
   
 
    FAT (NOLOAD) :
    {
      . = ALIGN(4);
      KEEP(*(FAT))
      . = ALIGN(4);
    } > FIB_region

/*
    .FAT (NOLOAD):
    {
      KEEP(*(.FAT))
    } > FIB_region
*/
    /* after that it's only debugging information. */
    

    /* This is the initialized data section
    The program executes knowing that the data is in the RAM
    but the loader puts the initial values in the FLASH (inidata).
    It is one task of the startup to copy the initial values from FLASH to RAM. */
    .data  : AT ( _sidata )
    {
	    . = ALIGN(4);
        /* This is used by the startup in order to initialize the .data secion */
        _sdata = . ;
        
        *(.data)
        *(.data.*)

	    . = ALIGN(4);
	    /* This is used by the startup in order to initialize the .data secion */
   	 _edata = . ;
    } >RAM_region
    
    

    /* This is the uninitialized data section */
    .bss :
    {
	    . = ALIGN(4);
        /* This is used by the startup in order to initialize the .bss secion */
        _sbss = .;
        
        *(.bss)
        *(COMMON)
        
	    . = ALIGN(4);
	    /* This is used by the startup in order to initialize the .bss secion */
   	 _ebss = . ;
    } >RAM_region
    
    PROVIDE ( end = _ebss );
    PROVIDE ( _end = _ebss );
    
    /* This is the user stack section 
    This is just to check that there is enough RAM left for the User mode stack
    It should generate an error if it's full.
     */
    ._usrstack :
    {
	    . = ALIGN(4);
        _susrstack = . ;
        
        . = . + _Minimum_Stack_Size ;
        
	    . = ALIGN(4);
        _eusrstack = . ;
    } >RAM_region
       
    __exidx_start = .;
    __exidx_end = .;

    /* remove the debugging information from the standard libraries */
    /DISCARD/ :
    {
     libc.a ( * )
     libm.a ( * )
     libgcc.a ( * )
     }

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }
    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
