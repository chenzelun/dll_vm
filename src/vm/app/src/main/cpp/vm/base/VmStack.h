//
// Created by 陈泽伦 on 12/6/20.
//

#ifndef VM_VMSTACK_H
#define VM_VMSTACK_H

#include "VmCommon.h"
#include "VmMethod.h"
#include "VmMemory.h"
#include "../../common/VmConstant.h"

#include <vector>
#include <set>

struct VmFrame {
    VmMethodContext vmc;
    VmFrame *pre = nullptr;
};

class VmStack {
protected:
    // call stack
    VmFrame *topFrame;

public:
    virtual void push(jobject caller, jmethodID method, jvalue *pResult, va_list param) = 0;

    virtual void pushWithoutParams(jmethodID method, jvalue *pResult) = 0;

    virtual void pop() = 0;

    inline VmFrame *getTopFrame() const {
        return this->topFrame;
    }

    virtual ~VmStack(){};
};

class VmRandomStack : public VmStack {
private:
    // memory
    VmMemory *memoryManager;

    // page
    uint32_t freePageCount;
    std::vector<uint64_t> freePages;
    std::vector<uint8_t *> freeMem;
    std::set<uint8_t *> fullPages;

    // PAGE_SIZE = 4KB
    const uint16_t DATA_COUNT_IN_PAGE = 64u;

public:
    VmRandomStack(uint16_t freePageSize, VmMemory *memoryManager);

    ~VmRandomStack();

    void push(jobject caller, jmethodID method, jvalue *pResult, va_list param) override;

    void pushWithoutParams(jmethodID method, jvalue *pResult) override;

    void pop() override;

private:
    VmFrame *newFrame(jobject caller, jmethodID method, jvalue *pResult, va_list param);

    void deleteFrame(VmFrame *frame);

    VmFrame *mallocFrame();

    void freeFrame(VmFrame *frame);
};


#endif //VM_VMSTACK_H
