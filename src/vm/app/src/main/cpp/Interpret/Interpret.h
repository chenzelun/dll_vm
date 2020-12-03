//
// Created by 陈泽伦 on 12/3/20.
//

#ifndef VM_INTERPRET_H
#define VM_INTERPRET_H

#include <map>
#include <string>
#include "../vm/VmMethod.h"

class CodeHandler {
public:
    virtual void run(VmMethodContext *vmc) = 0;
};

class Interpret {
protected:
    std::map<uint32_t, CodeHandler *> codeMap;

public:
    virtual void run(VmMethodContext *vmc) = 0;
};

#endif //VM_INTERPRET_H
