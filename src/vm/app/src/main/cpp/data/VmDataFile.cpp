//
// Created by 陈泽伦 on 2020/11/5.
//

#include "VmDataFile.h"
#include "../common/Util.h"
#include "../common/VmConstant.h"

VDF_KeyValueData::VDF_KeyValueData(const uint8_t *pr) {
    this->key = nullptr;
    this->val = nullptr;
    this->reset(pr);
}

const char *VDF_KeyValueData::getKey() const {
    return this->key->data;
}

const char *VDF_KeyValueData::getVal() const {
    return this->val->data;
}


void VDF_KeyValueData::reset(const uint8_t *pr) {
    this->key = (VDF_String *) pr;
    this->val = (VDF_String *) (pr + this->key->data_size + 0x04);
}

VDF_FileData::VDF_FileData(const uint8_t *pr) {
    this->data = nullptr;
    this->data_size = 0;
    this->name = nullptr;
    this->reset(pr);
}

const char *VDF_FileData::getName() const {
    return this->name->data;
}

uint32_t VDF_FileData::getDataSize() const {
    return this->data_size;
}

const uint8_t *VDF_FileData::getData() const {
    return this->data;
}

void VDF_FileData::reset(const uint8_t *pr) {
    this->name = (VDF_String *) pr;
    pr += this->name->data_size + 0x04;
    this->data_size = *(uint32_t *) pr;
    this->data = pr + 0x04;
}

VmDataFile::VmDataFile(const uint8_t *pr, uint32_t fileSize) {
    this->base = pr;
    this->end = pr + fileSize;
    this->header = (VDF_Header *) this->end - 1;
    this->index = (VDF_Index *) this->header - this->header->index_size;

    LOG_D("fileSize: %u", fileSize);
    LOG_D("index_size: %u", header->index_size);
    for (int i = 0; i < header->index_size; ++i) {
        const VDF_Index *cur = this->index + i;
        bool is_kv = cur->type == VDF_DataType::TYPE_KEY_VALUE;
        LOG_D("index[%d].type: %s", i, is_kv ? "Key-Value" : "File");
        LOG_D("index[%d].data_off: %u", i, cur->data_off);
        if (is_kv) {
            const VDF_KeyValueData data(this->base + cur->data_off);
            LOG_D("index[%s]: %s", data.getKey(), data.getVal());
        } else {
            const VDF_FileData data(this->base + cur->data_off);
            LOG_D("index[%d]-f-name: %s", i, data.getName());
            LOG_D("index[%d]-f-size: %u", i, data.getDataSize());
        }
    }
}

bool VmDataFile::findValByKey(const std::string &key, VDF_KeyValueData &retVal) const {
    for (int offset = 0; offset < this->header->index_size; offset++) {
        if (this->index[offset].type != VDF_DataType::TYPE_KEY_VALUE) {
            continue;
        }
        retVal.reset(this->base + this->index[offset].data_off);
        if (key == retVal.getKey()) {
            return true;
        }
    }
    return false;
}

bool VmDataFile::findFileByName(const std::string &key, VDF_FileData &retVal) const {
    for (int offset = 0; offset < this->header->index_size; offset++) {
        if (this->index[offset].type != VDF_DataType::TYPE_FILE) {
            continue;
        }
        retVal.reset(this->base + this->index[offset].data_off);
        if (key == retVal.getName()) {
            LOG_D("findFileByName: %s, size: %u, offset: %u,success.",
                  retVal.getName(), retVal.getDataSize(), this->index[offset].data_off);
            return true;
        }
    }
    LOG_D("findFileByName: %s, failure.", key.data());
    return false;
}
