//
// Created by 陈泽伦 on 12/3/20.
//

#ifndef VM_INTERPRET_H
#define VM_INTERPRET_H

#include <map>
#include <string>
#include "../base/VmMethod.h"
#include "VmMethodCaller.h"

class CodeHandler {
public:
    virtual void run(VmMethodContext *vmc) = 0;
};

class Interpret {
protected:
    std::map<uint32_t, CodeHandler *> codeMap;

public:
    virtual void run(VmMethodContext *vmc) = 0;

    virtual ~Interpret(){};
};

#endif //VM_INTERPRET_H
