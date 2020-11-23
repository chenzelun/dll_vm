//
// Created by 陈泽伦 on 11/16/20.
//

#include "VmKeyFuncCodeFile.h"

VmKeyFuncCodeFile::VmKeyFuncCodeFile(const uint8_t *pr, uint32_t fileSize) {
    this->base = pr;
    this->end = this->base + fileSize;

    this->header = (VKFC_Header *) this->end - 1;
    VKFC_Index *index = (VKFC_Index *) this->header - this->header->index_size;

    for(int off = 0; off< this->header->index_size; off++){
        uint32_t method_id = index[off].method_id;
        auto* c = (VKFC_Code *) (this->base + index[off].code_offset);
        this->code[method_id] = c->code;
    }
}

const uint8_t *VmKeyFuncCodeFile::getCode(uint32_t method_id) const {
    auto it = this->code.find(method_id);
    if(it!= this->code.end()){
        return it->second;
    }
    return nullptr;
}

