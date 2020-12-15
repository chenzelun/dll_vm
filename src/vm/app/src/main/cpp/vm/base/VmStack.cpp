//
// Created by 陈泽伦 on 12/6/20.
//

#include "VmStack.h"
#include <cstdlib>

void
VmRandomStack::push(
        jobject caller, jmethodID method, jvalue *pResult, va_list param) {
    VmFrame *frame = this->newFrame(caller, method, pResult, param);
#if defined(VM_DEBUG_FULL)
    LOG_D_VM("frame: %p", frame);
    LOG_D_VM("\n**********************************  "
             "enter vm: %s"
             "  **********************************", frame->vmc.method->name);
#else
    LOG_D("enter vm: %s", frame->vmc.method->name);
#endif
    frame->pre = this->topFrame;
    this->topFrame = frame;
}

void VmRandomStack::pushWithoutParams(jmethodID method, jvalue *pResult) {
    VmFrame *frame = this->mallocFrame();
    frame->vmc.resetWithoutParams(method, pResult);
#if defined(VM_DEBUG_FULL)
    LOG_D_VM("frame: %p", frame);
    LOG_D_VM("\n**********************************  "
             "enter vm: %s"
             "  **********************************", frame->vmc.method->name);
#else
    LOG_D("enter vm: %s", frame->vmc.method->name);
#endif
    frame->pre = this->topFrame;
    this->topFrame = frame;
}

void VmRandomStack::pop() {
    assert(this->topFrame != nullptr);
    VmFrame *frame = this->topFrame;
#if defined(VM_DEBUG_FULL)
    LOG_D_VM("frame: %p", frame);
    LOG_D_VM("\n**********************************  "
             "exit  vm: %s"
             "  **********************************", frame->vmc.method->name);
#else
    LOG_D("exit  vm: %s", frame->vmc.method->name);
#endif
    this->topFrame = frame->pre;
    this->deleteFrame(frame);
}

VmFrame *VmRandomStack::newFrame(
        jobject caller, jmethodID method, jvalue *pResult, va_list param) {
    VmFrame *frame = this->mallocFrame();
    frame->vmc.reset(caller, method, pResult, param);
    frame->pre = nullptr;
    return frame;
}

void VmRandomStack::deleteFrame(VmFrame *frame) {
    this->topFrame->vmc.curException = frame->pre->vmc.curException;
    frame->pre = nullptr;
    frame->vmc.release();
    this->freeFrame(frame);
}

VmRandomStack::VmRandomStack(uint16_t freePageSize, VmMemory *memoryManager) {
    this->memoryManager = memoryManager;
    assert(this->memoryManager != nullptr);
    this->freePageCount = freePageSize;
    this->topFrame = nullptr;
    for (int i = 0; i < freePageSize; ++i) {
        this->freeMem.push_back(this->memoryManager->malloc());
        this->freePages.push_back(0UL);
    }
}

VmRandomStack::~VmRandomStack() {
    for (auto it:this->fullPages) {
        this->memoryManager->free(it);
    }
    for (auto it:this->freeMem) {
        this->memoryManager->free(it);
    }
}

VmFrame *VmRandomStack::mallocFrame() {
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
    LOG_D_VM("memory: %p, num: %u", this->freeMem[d], num);
    this->freePages[d] |= r;
    uint8_t *ret = this->freeMem[d];
    if (this->freePages[d] == (~0x0UL)) {
        fullPages.insert(this->freeMem[d]);
        LOG_D_VM("add a full page: %p", this->freeMem[d]);
        if (freePages.size() == freePageCount) {
            this->freeMem[d] = this->memoryManager->malloc();
            this->freePages[d] = 0UL;
        } else {
            this->freePages.erase(this->freePages.begin() + d);
            this->freeMem.erase(this->freeMem.begin() + d);
        }
    }
    return (VmFrame *) (ret + (offset << 6u));
}

void VmRandomStack::freeFrame(VmFrame *frame) {
    uint32_t offset = ((uint64_t) frame & 0x3fu);
    uint64_t r = 1UL << offset;
    auto *f = (uint8_t *) frame;
    for (int i = 0; i < this->freePages.size(); ++i) {
        uint8_t *b = this->freeMem[i];
        if (b <= f && f < b + 0x1000u) {
            this->freePages[i] &= ~r;
            LOG_D_VM("freeCache a frame: %p, off: %u, r: 0x%016lu", this->freeMem[i], offset, r);
            if (this->freePages[i] == 0u &&
                this->freePages.size() > this->freePageCount) {
                this->memoryManager->free(this->freeMem[i]);
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
    LOG_D_VM("freeCache a frame from full pages.");
}
