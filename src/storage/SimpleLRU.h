#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>
// #include "/home/pavel/prog/afina/include/afina/Storage.h"

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024)
        : _max_size(max_size)
        , _size(0)
        , _lru_head(nullptr)
        , _lru_end(nullptr)
    {
    }

    virtual ~SimpleLRU() {
        _lru_index.clear();
        FreeEnoughMemory(_max_size - 1);
        // _lru_head.reset(); // TODO: Here is stack overflow
    }

    // Implements Afina::Storage interface
    virtual bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    virtual bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    virtual bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node * prev;
        std::unique_ptr<lru_node> next;
    };

    // Create new node in the head of list
    bool AddNode(const std::string & key, const std::string & value);

    // Move node to the head of list
    void MoveInHead(lru_node & node);

    // Get anough memory
    void FreeEnoughMemory(int size);

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _size;
    std::size_t _max_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node * _lru_end;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    using cash_map = std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>;

    cash_map _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
