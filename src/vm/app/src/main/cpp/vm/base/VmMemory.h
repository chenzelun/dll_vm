//
// Created by 陈泽伦 on 12/8/20.
//

#ifndef VM_VMMEMORY_H
#define VM_VMMEMORY_H

#include "VmCommon.h"
#include "../../common/VmConstant.h"

#include <set>

class VmMemory {
public:
    virtual uint8_t *malloc() = 0;

    virtual void free(void *p) = 0;
};

class VmRandomMemory : public VmMemory {
private:
    // buffer
    uint8_t *base;

    // page
    uint32_t maxPageCount;
    std::set<uint32_t> freePages;
    std::set<uint32_t> fullPages;


public:
    VmRandomMemory(uint64_t memSize);

    ~VmRandomMemory();

    uint8_t *malloc() override;

    void free(void *p) override;


private:
    void freeToSystem();

    uint8_t *memNum2Mem(uint32_t memNum) const;

    uint32_t mem2MemNum(const uint8_t *p) const;
};


#endif //VM_VMMEMORY_H
