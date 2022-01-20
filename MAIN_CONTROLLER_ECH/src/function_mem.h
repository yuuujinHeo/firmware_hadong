#ifndef _FUNCTION_MEM_H
#define _FUNCTION_MEM_H

unsigned int MEM_Read(int addr_offset);
void MEM_Write(int addr_offset, unsigned int value);


void MEM_InitiateOperation(void);
void MEM_WriteWord(void* address, unsigned int data);
void MEM_WriteQuadWord(void* address, unsigned int data0, unsigned int data1, unsigned int data2, unsigned int data3);
void MEM_WriteRow();
void MEM_ErasePage(void* address);
void MEM_EraseFlash(void);
#endif