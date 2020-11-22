//
// Created by 陈泽伦 on 2020/11/5.
//

#ifndef VM_VMDATAFILE_H
#define VM_VMDATAFILE_H

#include <stdint.h>
#include <stdlib.h>
#include <string>


struct VDF_Header {
    uint32_t index_size;
    uint8_t signature[20];
    uint32_t checksum;
};

enum VDF_DataType {
    TYPE_KEY_VALUE = 1,
    TYPE_FILE = 2,
};

struct VDF_Index {
    uint32_t type;
    uint32_t data_off;
};

struct VDF_String {
    uint32_t data_size;
    char data[0];
};

class VDF_KeyValueData {
private:
    VDF_String *key;
    VDF_String *val;

public:
    VDF_KeyValueData() {};

    VDF_KeyValueData(const uint8_t *pr);

    void reset(const uint8_t *pr);

    const char *getKey() const;

    const char *getVal() const;
};

class VDF_FileData {
private:
    VDF_String *name;
    uint32_t data_size;
    const uint8_t *data;

public:
    VDF_FileData() {};

    VDF_FileData(const uint8_t *pr);

    void reset(const uint8_t *pr);

    const char *getName() const;

    uint32_t getDataSize() const;

    const uint8_t * getData() const;
};

class VmDataFile {
private:
    const uint8_t *base;
    const uint8_t *end;
    VDF_Header *header;
    VDF_Index *index;

public:
    VmDataFile(const uint8_t *pr, uint32_t fileSize);

    bool findValByKey(const std::string &key, VDF_KeyValueData &retVal) const;

    bool findFileByName(const std::string &key, VDF_FileData &retVal) const;
};


#endif //VM_VMDATAFILE_H
