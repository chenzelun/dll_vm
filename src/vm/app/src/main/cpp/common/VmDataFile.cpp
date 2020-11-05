//
// Created by 陈泽伦 on 2020/11/5.
//

#include "VmDataFile.h"
#include "Util.h"
#include "VmConstant.h"

VmKeyValueData::VmKeyValueData(const uint8_t *pr) {
    this->reset(pr);
}

const char *VmKeyValueData::getKey() const {
    return this->key->data;
}

const char *VmKeyValueData::getVal() const {
    return this->val->data;
}


void VmKeyValueData::reset(const uint8_t *pr) {
    this->key = (VmString *) pr;
    this->val = (VmString *) (pr + this->key->data_size + 0x04);
}

VmFileData::VmFileData(const uint8_t *pr) {
    this->reset(pr);
}

const char *VmFileData::getName() const {
    return this->name->data;
}

uint32_t VmFileData::getDataSize() const {
    return this->data_size;
}

const char *VmFileData::getData() const {
    return this->data;
}

void VmFileData::reset(const uint8_t *pr) {
    this->name = (VmString *) pr;
    this->data_size = *(uint32_t *) (pr + this->name->data_size + 0x04);
    this->data = (char *) (pr + 0x04);
}

VmDataFile::VmDataFile(const uint8_t *pr, uint32_t fileSize) {
    this->base = pr;
    this->end = pr + fileSize;
    this->header = (VmHeader *) (this->end - sizeof(VmHeader));
    this->index = (VmIndex *) this->header - this->header->index_size;
}

bool VmDataFile::findValByKey(const std::string &key, VmKeyValueData &retVal) const {
    for (int offset = 0; offset < this->header->index_size; offset++) {
        if (this->index[offset].type != VmDataType::TYPE_KEY_VALUE) {
            continue;
        }
        retVal.reset(this->base + this->index[offset].data_off);
        if (key == retVal.getKey()) {
            return true;
        }
    }
    return false;
}

bool VmDataFile::findFileByName(const std::string &key, VmFileData &retVal) const {
    for(int offset=0; offset< this->header->index_size; offset++){
        if (this->index[offset].type != VmDataType::TYPE_FILE) {
            continue;
        }
        retVal.reset(this->base + this->index[offset].data_off);
        if (key == retVal.getName()) {
            return true;
        }
    }
    return false;
}
