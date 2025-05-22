#pragma once
#include <atomic>
#include <vector>
#include <algorithm>  // 添加algorithm头文件
#include <memory>
#include <thread>

namespace lockfree {

constexpr int HP_MAX_THREADS = 128;
constexpr int HP_MAX_HPS = 2;    // 每个线程最大hazard pointer数量
constexpr int HP_RETIRE_THRESHOLD = 2;  // 添加退休阈值常量

class HazardPointer {
public:
    std::atomic<void*> ptr;
    std::atomic<std::thread::id> id;
    
    HazardPointer() : ptr(nullptr), id(std::thread::id()) {}
};

class HPRecType {
public:
    std::atomic<HPRecType*> next;
    std::atomic<int> active;
    HazardPointer hp[HP_MAX_HPS];
    std::vector<std::pair<void*, std::function<void(void*)>>> retired; // 存储指针和删除器
    
    HPRecType() : next(nullptr), active(0) {
        for(auto& h : hp) {
            h.ptr.store(nullptr, std::memory_order_relaxed);
            h.id.store(std::thread::id(), std::memory_order_relaxed);
        }
    }
};

class HazardPointerManager {
private:
    static thread_local HPRecType* local_rec;
    static std::atomic<HPRecType*> head;
    static std::atomic<size_t> list_len;

public:
    static HazardPointer* acquire() {
        if (!local_rec) {
            local_rec = new HPRecType();
            HPRecType* old_head = head.load(std::memory_order_acquire);
            do {
                local_rec->next.store(old_head, std::memory_order_relaxed);
            } while (!head.compare_exchange_weak(old_head, local_rec,
                                                std::memory_order_release,
                                                std::memory_order_acquire));
            list_len.fetch_add(1, std::memory_order_relaxed);
        }
        
        for (int i = 0; i < HP_MAX_HPS; ++i) {
            std::thread::id old_id = std::thread::id();
            if (local_rec->hp[i].id.compare_exchange_strong(
                old_id, std::this_thread::get_id(),
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
                return &local_rec->hp[i];
            }
        }
        return nullptr;
    }

    static void release(HazardPointer* hp) {
        hp->ptr.store(nullptr, std::memory_order_release);
        hp->id.store(std::thread::id(), std::memory_order_release);
    }

    // 添加HazardPointerGuard RAII包装类
    template <typename T>
    class HazardPointerGuard {
    public:
        explicit HazardPointerGuard(T* ptr) {
            hp = HazardPointerManager::acquire();
            if (hp) {
                hp->ptr.store(ptr, std::memory_order_release);
            }
        }

        ~HazardPointerGuard() {
            if (hp) {
                HazardPointerManager::release(hp);
            }
        }

        HazardPointerGuard(const HazardPointerGuard&) = delete;
        HazardPointerGuard& operator=(const HazardPointerGuard&) = delete;

    private:
        HazardPointer* hp = nullptr;
    };

    // 添加is_protected检查方法
    template <typename T>
    static bool is_protected(T* ptr) {
        auto& rec = *local_rec;
        for (auto& h : rec.hp) {
            if (h.ptr.load(std::memory_order_acquire) == ptr) {
                return true;
            }
        }
        return false;
    }

    template<typename T, typename Deleter = std::default_delete<T>>
    static void retire(T* ptr, Deleter deleter = {}) {
        // 延迟删除逻辑，使用lambda包装删除器
        local_rec->retired.emplace_back(
            ptr,
            [deleter](void* p) { deleter(static_cast<T*>(p)); }
        );
        if (local_rec->retired.size() >= HP_RETIRE_THRESHOLD) {
            scan<T>();
        }
    }

private:
    template<typename T>
    static void scan() {
        std::vector<void*> plist = get_protected_pointers();
        
        auto it = local_rec->retired.begin();
        while (it != local_rec->retired.end()) {
            if (std::find(plist.begin(), plist.end(), *it) == plist.end()) {
                delete static_cast<T*>(*it); 
                it = local_rec->retired.erase(it);
            } else {
                ++it;
            }
        }
    }

    static std::vector<void*> get_protected_pointers() {
        std::vector<void*> result;
        HPRecType* curr = head.load(std::memory_order_acquire);
        while (curr) {
            if (curr->active.load(std::memory_order_acquire)) {
                for (auto& h : curr->hp) {
                    void* p = h.ptr.load(std::memory_order_acquire);
                    if (p) {
                        result.push_back(p);
                    }
                }
            }
            curr = curr->next.load(std::memory_order_acquire);
        }
        return result;
    }
};

thread_local HPRecType* HazardPointerManager::local_rec = nullptr;
std::atomic<HPRecType*> HazardPointerManager::head(nullptr);
std::atomic<size_t> HazardPointerManager::list_len(0);

} // namespace lockfree
