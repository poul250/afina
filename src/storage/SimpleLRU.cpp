#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    return _lru_index.count(key) == 0 ? PutIfAbsent(key, value) : Set(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    if (_lru_index.count(key) == 0 && key.size() + value.size() <= _max_size) {
        FreeEnoughMemory(key.size() + value.size());
        AddNode(key, value);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(key);
    if (it != _lru_index.end() && key.size() + value.size() <= _max_size) {
        lru_node & node = it->second;
        MoveInHead(node);
        FreeEnoughMemory(value.size() - node.value.size());

        _size += value.size() - node.value.size();
        node.value = value;
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        lru_node & node = it->second;
        _size -= node.key.size() + node.value.size();
        _lru_index.erase(it);

        auto & left = _lru_head.get() == &node ? _lru_head : node.prev->next;

        auto tmp = std::move(left);
        left = std::move(tmp->next);
        if (left != nullptr)
            left->prev = tmp->prev;
        else
            _lru_end = tmp->prev;

        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        auto & node = it->second.get();
        value = node.value;
        MoveInHead(node);
        return true;
    }
    return false;
}

bool SimpleLRU::AddNode(const std::string & key, const std::string & value)
{
    lru_node * node = new lru_node{key, value, nullptr, std::move(_lru_head)};
    auto res = _lru_index.emplace(std::make_pair(std::ref(node->key), std::ref(*node)));
    if (!res.second) {
        _lru_head = std::move(node->next);
        delete node;
        return false;
    }

    _size += key.size() + value.size();
    _lru_head = std::unique_ptr<lru_node>(node);

    if (node->next != nullptr)
        node->next->prev = node;
    else
        _lru_end = node;
    return true;
}

void SimpleLRU::MoveInHead(lru_node & node)
{
    if (_lru_head.get() != &node) {
        std::unique_ptr<lru_node> tmp = std::move(node.prev->next);

        if (node.next != nullptr)
            node.next->prev = node.prev;
        else
            _lru_end = node.prev;

        node.prev->next = std::move(node.next);

        node.prev = nullptr;
        _lru_head->prev = &node;

        node.next = std::move(_lru_head);
        _lru_head = std::move(tmp);
    }
}

void SimpleLRU::FreeEnoughMemory(int size)
{
    while (_size + size > _max_size) {
        // Delete(_lru_end->key);
        auto del = std::move(_lru_end == _lru_head.get() ?
            _lru_head : _lru_end->prev->next);
        _lru_index.erase(del->key);
        _lru_end = del->prev;
        _size -= del->key.size() + del->value.size();
    }
}

} // namespace Backend
} // namespace Afina
