//
// Created by 陈泽伦 on 12/8/20.
//

#ifndef VM_VMMEMORY_H
#define VM_VMMEMORY_H

#include "VmCommon.h"
#include "../common/VmConstant.h"

#include <set>

class VmMemory {
private:
    // buffer
    uint8_t *base;

    // page
    uint32_t maxPageCount;
    std::set<uint32_t> freePages;
    std::set<uint32_t> fullPages;


public:
    VmMemory(uint64_t memSize);

    ~VmMemory();

    uint8_t *malloc();

    void free(void *p);


private:
    void freeToSystem();

    uint8_t *memNum2Mem(uint32_t memNum)const;

    uint32_t mem2MemNum(const uint8_t *p)const;
};



#endif //VM_VMMEMORY_H
