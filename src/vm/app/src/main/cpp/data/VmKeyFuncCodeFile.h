//
// Created by 陈泽伦 on 11/16/20.
//

#ifndef VM_VMKEYFUNCCODEFILE_H
#define VM_VMKEYFUNCCODEFILE_H

#include <stdint.h>
#include <stdlib.h>
#include <map>

struct VKFC_Header {
    uint32_t index_size;
    uint8_t signature[20];
    uint32_t checksum;
};

struct VKFC_Index {
    uint32_t method_id;
    uint32_t code_offset;
};

struct VKFC_Code {
    uint8_t code[0];
};

class VmKeyFuncCodeFile {
private:
    VKFC_Header *header;
    std::map<uint32_t, uint8_t *> code;

    const uint8_t *base;
    const uint8_t *end;

public:
    VmKeyFuncCodeFile(const uint8_t *pr, uint32_t fileSize);

    const uint8_t *getCode(uint32_t method_id) const;
};


#endif //VM_VMKEYFUNCCODEFILE_H
