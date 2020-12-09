//
// Created by 陈泽伦 on 12/6/20.
//

#ifndef VM_VMSTACK_H
#define VM_VMSTACK_H

#include "VmCommon.h"
#include "VmMethod.h"
#include "../common/VmConstant.h"
#include "VmMemory.h"

#include <vector>
#include <set>

struct VmFrame {
    VmMethodContext vmc;
    VmFrame *pre = nullptr;
};

class VmStack {
private:
    // page
    uint32_t freePageCount;
    std::vector<uint64_t> freePages;
    std::vector<uint8_t *> freeMem;
    std::set<uint8_t *> fullPages;

    // call stack
    VmFrame *topFrame;

    // tmp data
    VmTempData methodTempData;

    // PAGE_SIZE = 4KB
    const uint16_t DATA_COUNT_IN_PAGE = 64u;

public:
    VmStack(uint16_t freePageSize, VmMemory *vmMemory);

    VmTempData *getTempDataBuf();

    VmMethodContext *push(
            jobject caller, const VmMethod *method, jvalue *pResult, va_list param);

    void pop();

private:

    VmFrame *mallocFrame();

    void freeFrame(VmFrame *frame);
};


#endif //VM_VMSTACK_H
