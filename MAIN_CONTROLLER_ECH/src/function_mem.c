#include "config_cocoa_general.h"
#include "function_mem.h"

const unsigned int MEM_ADDR = 0x1D01F000;


unsigned int i,j;
unsigned int rowbuff[512];


unsigned int MEM_Read(int addr_offset){
    return *(unsigned int*)(PA_TO_KVA0(MEM_ADDR+addr_offset*4));
}

void MEM_Write(int addr_offset, unsigned int value){
    MEM_WriteWord(MEM_ADDR+addr_offset*4, value);
}






void MEM_InitiateOperation(void){
    int int_status; // storage for current Interrupt Enable state
    int dma_susp; // storage for current DMA state
    // Disable Interrupts
    asm volatile("di %0" : "=r"(int_status));
    // Disable DMA
    if(!(dma_susp=DMACONbits.SUSPEND)){
        DMACONSET=_DMACON_SUSPEND_MASK; // suspend
        while((DMACONbits.DMABUSY)); // wait to be actually suspended
    }
    NVMKEY = 0x0;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    NVMCONSET = 1 << 15; // must be an atomic instruction
    // Restore DMA
    if(!dma_susp){
        DMACONCLR=_DMACON_SUSPEND_MASK; // resume DMA activity
    }
    // Restore Interrupts
    if(int_status & 0x00000001){
        asm volatile("ei");
    }
}

void MEM_WriteWord(void* address, unsigned int data){
    // Set up Address and Data Registers
    NVMADDR = (unsigned int)address; // physical address
    NVMDATA0 = data;
    // set the operation, assumes WREN = 0
    NVMCONbits.NVMOP = 0x1; // NVMOP for Word programming
    // Enable Flash for write operation and set the NVMOP
    NVMCONbits.WREN = 1;
    // Start programming
    MEM_InitiateOperation(); // see Example 52-1
    // Wait for WR bit to clear
    while(NVMCONbits.WR);
    // Disable future Flash Write/Erase operations
    NVMCONbits.WREN = 0;
    // Check Error Status
    if(NVMCON & 0x3000) // mask for WRERR and LVDERR
    {
//        printl_uart("Error at MEM_WriteWord\n");
    }
}

void MEM_WriteQuadWord(void* address, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3){
    // Set up Address and Data Registers
    NVMADDR = (unsigned int)address; // physical address
    NVMDATA0 = data0;
    NVMDATA1 = data1;
    NVMDATA2 = data2;
    NVMDATA3 = data3;
    // set the operation, assumes WREN = 0
    NVMCONbits.NVMOP = 0x2; // NVMOP for Quad Word programming
    // Enable Flash for write operation and set the NVMOP
    NVMCONbits.WREN = 1;
    // Start programming
    MEM_InitiateOperation(); // see Example 52-1
    // Wait for WR bit to clear
    while(NVMCONbits.WR);
    // Disable future Flash Write/Erase operations
    NVMCONbits.WREN = 0;
    // Check Error Status
    if(NVMCON & 0x3000) // mask for WRERR and LVDERR bits
    {
//        printl_uart("Error at MEM_WriteQuadWord\n");
    }
}

void MEM_WriteRow(){
//    // put some data in the source buffer
//    for (i = 0; i < (sizeof(rowbuff) * sizeof (int)); i++)
//    ((char *)rowbuff)[i] = i;
//    // set destination row address
//    NVMADDR = 0x1D008000; // row physical address
//    // set source address. Must be converted to a physical address.
//    NVMSRCADDR = (unsigned int)((int)rowbuff & 0x1FFFFFFF);
//    // define Flash operation
//    NVMCONbits.NVMOP = 0x3; // NVMOP for Row programming
//    // Enable Flash Write
//    NVMCONbits.WREN = 1;
//    // commence programming
//    MEM_InitiateOperation(); // see Example 52-1
//    // Wait for WR bit to clear
//    while(NVMCONbits.WR);
//    // Disable future Flash Write/Erase operations
//    NVMCONbits.WREN = 0;
//    // Check Error Status
//    if(NVMCON & 0x3000) // mask for WRERR and LVDERR bits
//    {
//        printl_uart("Error at MEM_WriteRow\n");
//    }
}

void MEM_ErasePage(void* address){
    // set destination page address
    NVMADDR = (unsigned int)address; // page physical address
    // define Flash operation
    NVMCONbits.NVMOP = 0x4; // NVMOP for Page Erase
    // Enable Flash Write
    NVMCONbits.WREN = 1;
    // commence programming
    MEM_InitiateOperation(); // see Example 52-1
    // Wait for WR bit to clear
    while(NVMCONbits.WR);
    // Disable future Flash Write/Erase operations
    NVMCONbits.WREN = 0;
    // Check Error Status
    if(NVMCON & 0x3000) // mask for WRERR and LVDERR bits
    {
//        printl_uart("Error at MEM_ErasePage\n");
    }
}

void MEM_EraseFlash(void){
    // define Flash operation
    NVMCONbits.NVMOP = 0x7;
    // Enable Flash Write
    NVMCONbits.WREN = 1;
    // commence programming
    MEM_InitiateOperation(); // see Example 52-1
    // Wait for WR bit to clear
    while(NVMCONbits.WR);
    // Disable future Flash Write/Erase operations
    NVMCONbits.WREN = 0;
    // Check Error Status
    if(NVMCON & 0x3000) // mask for WRERR and LVDERR bits
    {
//        printl_uart("Error at MEM_EraseFlash\n");
    }
}