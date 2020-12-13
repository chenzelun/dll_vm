//
// Created by 陈泽伦 on 12/8/20.
//

#include "VmMemory.h"
#include "../../common/Util.h"
#include <sys/mman.h>
#include <cstdlib>
#include <cerrno>

VmRandomMemory::VmRandomMemory(uint64_t memSize) {
    assert((memSize & 0xfffu) == 0);
    this->base = (uint8_t *) mmap(nullptr,
                                  memSize,
                                  (uint) PROT_READ | (uint) PROT_WRITE,
                                  (uint) MAP_PRIVATE | (uint) MAP_ANONYMOUS,
                                  -1, 0);
    if (this->base == MAP_FAILED) {
        LOG_E("VmRandomMemory mmap this->base fail.");
        LOG_E("error: %s", strerror(errno));
        throw VMException("VmRandomMemory mmap this->base fail.");
    }
    this->maxPageCount = memSize >> 12u;
    LOG_D("memory size: %lu, base: %p", memSize, this->base);
}

VmRandomMemory::~VmRandomMemory() {
    munmap(this->base, this->maxPageCount << 12u);
}

uint8_t *VmRandomMemory::malloc() {
    uint32_t memNum;
    if (!this->freePages.empty()) {
        // malloc from cache.
        memNum = *this->freePages.begin();
        this->freePages.erase(memNum);
    } else {
        // choose a memory unused.
        do {
            memNum = random() % this->maxPageCount;
        } while (this->fullPages.find(memNum) != this->fullPages.end());
    }
    this->fullPages.insert(memNum);
    LOG_D("VmRandomMemory::malloc: %p, num: %u", this->memNum2Mem(memNum), memNum);
    return this->memNum2Mem(memNum);
}

void VmRandomMemory::free(void *p) {
    uint32_t memNum = this->mem2MemNum((uint8_t *) p);
    LOG_D("VmRandomMemory::free: %p, num: %u", p, memNum);
    this->fullPages.erase(memNum);
    this->freePages.insert(memNum);
    this->freeToSystem();
}

void VmRandomMemory::freeToSystem() {
    if (this->freePages.size() < VM_CONFIG::VM_MEMORY_FREE_PAGE_SIZE) {
        return;
    }
    LOG_D("freeToSystem");
    std::set<uint32_t> needDel;
    uint32_t idx = 0;
    for (auto it = this->freePages.begin(); it != this->freePages.end(); it++, idx++) {
        if (idx & 0x01u) {
            continue;
        }
        needDel.insert(*it);
        if (munmap(this->memNum2Mem(*it), 0x1000u) == -1) {
            LOG_E("can't munmap at: %p", this->memNum2Mem(*it));
            LOG_E("error: %s", strerror(errno));
            throw VMException("can't munmap.");
        }
        LOG_D("munmap: %p", this->memNum2Mem(*it));
    }

    for (auto it : needDel) {
        this->fullPages.erase(it);
        if (mmap(this->memNum2Mem(it),
                 0x1000u,
                 (uint) PROT_WRITE | (uint) PROT_READ,
                 (uint) MAP_ANONYMOUS | (uint) MAP_FIXED,
                 -1, 0) == MAP_FAILED) {
            LOG_E("can't mmap at: %p", this->memNum2Mem(it));
            LOG_E("error: %s", strerror(errno));
            throw VMException("can't mmap.");
        }
        LOG_D("mmap: %p", this->memNum2Mem(it));
    }
}

uint8_t *VmRandomMemory::memNum2Mem(uint32_t memNum) const {
    return this->base + (memNum << 12u);
}

uint32_t VmRandomMemory::mem2MemNum(const uint8_t *p) const {
    return (uint64_t) (p - this->base) >> 12u;
}
