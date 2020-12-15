//
// Created by 陈泽伦 on 12/13/20.
//

#include "VmCache.h"
#include "../common/Util.h"
#include <cstdlib>

uint8_t *VmLinearCache::mallocCache(uint32_t key, uint32_t count) {
    LOG_D_VM("key: %u, count: %u", key, count);
    assert(key != 0);
    auto *buf = this->cache.find(key)->second;
    assert(0 < count && count <= 0xfff / buf->size);
    uint8_t *memory;
    if (count > this->full(buf)) {
        // not enough
        memory = this->memoryManager->malloc();
        buf->bufList.push_back(memory);
        buf->lastCount.push_back(count);
        return memory;
    } else {
        uint32_t oldCount = buf->lastCount.back();
        buf->lastCount.pop_back();
        memory = buf->bufList.back() + oldCount * buf->size;
        buf->lastCount.push_back(oldCount + count);
    }
    LOG_D_VM("malloc new cache: %p", memory);
    return memory;
}

void VmLinearCache::freeCache(uint32_t key, uint32_t count) {
    LOG_D_VM("key: %u, count: %u", key, count);
    assert(key != 0);
    auto *buf = this->cache.find(key)->second;
    uint32_t oldCount = buf->lastCount.back();
    buf->lastCount.pop_back();
    assert(count <= oldCount);
    if (count == oldCount) {
        if (buf->bufList.size() == 1) {
            buf->lastCount.push_back(0u);
        } else {
            this->memoryManager->free(buf->bufList.back());
            buf->bufList.pop_back();
        }
    } else {
        buf->lastCount.push_back(oldCount - count);
    }
}

uint32_t VmLinearCache::newCacheType(uint32_t bufSize) {
    LOG_D_VM("bufSize: %u", bufSize);
    uint32_t key;
    do {
        key = random();
        LOG_D_VM("trying cache type: %u", key);
    } while (this->cache.find(key) != this->cache.end() || key == 0);
    auto *buf = new VmLinearCacheBuffer();
    buf->size = bufSize;
    buf->bufList.push_back(this->memoryManager->malloc());
    buf->lastCount.push_back(0u);
    this->cache[key] = buf;
    LOG_D_VM("create new cache type: %u", key);
    return key;
}

VmLinearCache::VmLinearCache(VmMemory *memoryManager) {
    this->memoryManager = memoryManager;
    assert(this->memoryManager != nullptr);
}
