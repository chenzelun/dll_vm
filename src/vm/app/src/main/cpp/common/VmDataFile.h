//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMDATAFILE_H
#define VM_VMDATAFILE_H

#include <stdint.h>
#include <stdlib.h>
#include <string>


struct VmHeader {
    uint32_t index_size;
    uint8_t signature[20];
    uint32_t checksum;
};

enum VmDataType {
    TYPE_KEY_VALUE = 1,
    TYPE_FILE = 2,
};

struct VmIndex {
    uint32_t type;
    uint32_t data_off;
};

struct VmString {
    uint32_t data_size;
    char data[0];
};

class VmKeyValueData {
private:
    VmString *key{};
    VmString *val{};

public:
    VmKeyValueData() {};

    VmKeyValueData(const uint8_t *pr);

    void reset(const uint8_t *pr);

    const char *getKey() const;

    const char *getVal() const;
};

class VmFileData {
private:
    VmString *name{};
    uint32_t data_size{};
    const char *data{};

public:
    VmFileData() {};

    VmFileData(const uint8_t *pr);

    void reset(const uint8_t *pr);

    const char *getName() const;

    uint32_t getDataSize() const;

    const char *getData() const;
};

class VmDataFile {
private:
    const uint8_t *base;
    const uint8_t *end;
    VmHeader *header;
    VmIndex *index;

public:
    VmDataFile(const uint8_t *pr, uint32_t fileSize);

    bool findValByKey(const std::string &key, VmKeyValueData &retVal) const;

    bool findFileByName(const std::string &key, VmFileData &retVal) const;

    ~VmDataFile(){
        free((void *) this->base);
    };
};


#endif //VM_VMDATAFILE_H
