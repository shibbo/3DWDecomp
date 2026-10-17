#pragma once

void* VESSEL_malloc(unsigned long size, int alignment);
void VESSEL_free(void* pMemory);
unsigned int VESSE_STACKINIT();
void VESSEL_STACKDISP(const char* pFile, unsigned int line, const char* pLabel);
