//
// Created by 陈泽伦 on 12/13/20.
//

#ifndef VM_VMCACHE_H
#define VM_VMCACHE_H

#include "stdint.h"
#include "VmMemory.h"

#include <map>
#include <list>

class VmCache {
public:
    virtual uint8_t *mallocCache(uint32_t key, uint32_t count) = 0;

    virtual void freeCache(uint32_t key, uint32_t count) = 0;

    virtual uint32_t newCacheType(uint32_t bufSize) = 0;

    virtual ~VmCache(){};
};

struct VmLinearCacheBuffer {
    uint32_t size;
    std::list<uint8_t *> bufList;
    std::list<uint32_t> lastCount;
};

class VmLinearCache : public VmCache {
private:
    VmMemory *memoryManager;
    std::map<uint32_t, VmLinearCacheBuffer *> cache;

public:
    VmLinearCache(VmMemory *memoryManager);

    uint8_t *mallocCache(uint32_t key, uint32_t count) override;

    void freeCache(uint32_t key, uint32_t count) override;

    uint32_t newCacheType(uint32_t bufSize) override;

private:
    inline uint32_t full(VmLinearCacheBuffer *buf) const {
        return 0xfff / buf->size - buf->lastCount.back();
    }
};

#endif //VM_VMCACHE_H
