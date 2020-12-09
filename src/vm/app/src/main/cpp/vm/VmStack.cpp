//
// Created by 陈泽伦 on 12/6/20.
//

#include "VmStack.h"
#include "VmMemory.h"
#include "../VmContext.h"
#include <cstdlib>

VmTempData *VmStack::getTempDataBuf() {
    memset(&this->methodTempData, 0, sizeof(this->methodTempData));
    return &this->methodTempData;
}

VmMethodContext *
VmStack::push(
        jobject caller, const VmMethod *method, jvalue *pResult, va_list param) {
    assert(method->code != nullptr);
    VmFrame *frame = this->mallocFrame();
    LOG_D("frame: %p", frame);
    frame->vmc.reset(caller, method, pResult, param);
    frame->pre = this->topFrame;
    this->topFrame = frame;
    return &frame->vmc;
}

void VmStack::pop() {
    assert(this->topFrame != nullptr);
    VmFrame *frame = this->topFrame;
    LOG_D("frame: %p", frame);
    this->topFrame = frame->pre;
    this->freeFrame(frame);
}

VmStack::VmStack(uint16_t freePageSize, VmMemory *vmMemory) {
    this->freePageCount = freePageSize;
    this->topFrame = nullptr;
    for (int i = 0; i < freePageSize; ++i) {
        this->freeMem.push_back(vmMemory->malloc());
        this->freePages.push_back(0UL);
    }
}

VmFrame *VmStack::mallocFrame() {
    uint64_t r;
    uint32_t d;
    uint32_t offset;
    uint32_t num;
    do {
        num = random() % (this->freePages.size() * this->DATA_COUNT_IN_PAGE);
        offset = num & 0x3fU;
        r = 1UL << offset;
        d = num >> 6u;
    } while (this->freePages[d] & r);
    LOG_D("memory: %p, num: %u", this->freeMem[d], num);
    this->freePages[d] |= r;
    uint8_t *ret = this->freeMem[d];
    if (this->freePages[d] == (~0x0UL)) {
        fullPages.insert(this->freeMem[d]);
        LOG_D("add a full page: %p", this->freeMem[d]);
        if (freePages.size() == freePageCount) {
            this->freeMem[d] = VM_CONTEXT::vm->getVmMemory()->malloc();
            this->freePages[d] = 0UL;
        } else {
            this->freePages.erase(this->freePages.begin() + d);
            this->freeMem.erase(this->freeMem.begin() + d);
        }
    }
    return (VmFrame *) (ret + (offset << 6u));
}

void VmStack::freeFrame(VmFrame *frame) {
    uint32_t offset = ((uint64_t) frame & 0x3fu);
    uint64_t r = 1UL << offset;
    auto *f = (uint8_t *) frame;
    for (int i = 0; i < this->freePages.size(); ++i) {
        uint8_t *b = this->freeMem[i];
        if (b <= f && f < b + 0x1000u) {
            this->freePages[i] &= ~r;
            LOG_D("free a frame: %p, off: %u, r: 0x%016lu", this->freeMem[i], offset, r);
            if (this->freePages[i] == 0u &&
                this->freePages.size() > this->freePageCount) {
                VM_CONTEXT::vm->getVmMemory()->free(this->freeMem[i]);
                this->freePages.erase(this->freePages.begin() + i);
                this->freeMem.erase(this->freeMem.begin() + i);
            }
            return;
        }
    }

    auto *t = (uint8_t *) ((uint64_t) frame & (~0xfffUL));
    if (this->fullPages.find(t) == this->fullPages.end()) {
        LOG_E("can't find the memory: %p", frame);
        throw VMException("can't find the memory");
    }
    this->fullPages.erase(t);
    this->freeMem.push_back(t);
    this->freePages.push_back(0xffffffffffffffffUL & (~r));
    LOG_D("free a frame from full pages.");
}
