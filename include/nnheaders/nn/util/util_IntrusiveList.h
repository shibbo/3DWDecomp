#pragma once

#include <nn/types.h>
#include <iterator>

namespace nn {
namespace util {
namespace detail {
class IntrusiveListImplementation;
};

class IntrusiveListNode {
public:
    IntrusiveListNode() {
        m_Prev = this;
        m_Next = this;
    }

    bool IsLinked() const { return m_Next != this; }
    IntrusiveListNode* GetPrev() { return m_Prev; }
    const IntrusiveListNode* GetPrev() const { return m_Prev; }
    IntrusiveListNode* GetNext() { return m_Next; }
    const IntrusiveListNode* GetNext() const { return m_Next; }

    void LinkPrev(IntrusiveListNode* pNode) { LinkPrev(pNode, pNode); }

    void LinkPrev(IntrusiveListNode* pFirst, IntrusiveListNode* pLast) {
        IntrusiveListNode* node = pLast->m_Prev;
        pFirst->m_Prev = m_Prev;
        node->m_Next = this;
        m_Prev->m_Next = pFirst;
        m_Prev = node;
    }

    void LinkNext(IntrusiveListNode* pNode) { LinkNext(pNode, pNode); }

    void LinkNext(IntrusiveListNode* pFirst, IntrusiveListNode* pLast) {
        IntrusiveListNode* node = pLast->m_Prev;
        pFirst->m_Prev = this;
        node->m_Next = m_Next;
        m_Next->m_Prev = node;
        m_Next = pFirst;
    }

    void Unlink() { Unlink(m_Next); }

    void Unlink(IntrusiveListNode* pLast) {
        IntrusiveListNode* node = pLast->m_Prev;
        m_Prev->m_Next = pLast;
        pLast->m_Prev = m_Prev;
        node->m_Next = this;
        m_Prev = node;
    }

    IntrusiveListNode* m_Prev;
    IntrusiveListNode* m_Next;
};

template <class T, IntrusiveListNode T::*Member, class Container = T, size_t ContainerSize = sizeof(Container)>
class IntrusiveListMemberNodeTraits {
public:
    static IntrusiveListNode& GetNode(T& rRef) { return rRef.*Member; }
    static const IntrusiveListNode& GetNode(const T& rRef) { return rRef.*Member; }

    static T& GetItem(IntrusiveListNode& rNode) {
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(&rNode) - GetOffset());
    }

    static const T& GetItem(const IntrusiveListNode& rNode) {
        return *reinterpret_cast<const T*>(reinterpret_cast<const char*>(&rNode) - GetOffset());
    }

    static uintptr_t GetOffset() {
        return reinterpret_cast<uintptr_t>(&((reinterpret_cast<T*>(0))->*Member));
    }
};

template <class T, class NodeTraits>
class IntrusiveList {
public:
    class const_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        explicit const_iterator(const IntrusiveListNode* pNode) : m_pNode(pNode) {}

        const T& operator*() const { return NodeTraits::GetItem(*m_pNode); }
        const T* operator->() const { return &NodeTraits::GetItem(*m_pNode); }

        const_iterator& operator++() {
            m_pNode = m_pNode->GetNext();
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator temporary(*this);
            ++(*this);
            return temporary;
        }

        const_iterator& operator--() {
            m_pNode = m_pNode->GetPrev();
            return *this;
        }

        bool operator==(const const_iterator& rOther) const { return m_pNode == rOther.m_pNode; }
        bool operator!=(const const_iterator& rOther) const { return !(*this == rOther); }

    private:
        const IntrusiveListNode* m_pNode;
    };

    class iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        explicit iterator(IntrusiveListNode* pNode) : m_pNode(pNode) {}

        T& operator*() const { return NodeTraits::GetItem(*m_pNode); }
        T* operator->() const { return &NodeTraits::GetItem(*m_pNode); }

        iterator& operator++() {
            m_pNode = m_pNode->GetNext();
            return *this;
        }

        iterator operator++(int) {
            iterator temporary(*this);
            ++(*this);
            return temporary;
        }

        iterator& operator--() {
            m_pNode = m_pNode->GetPrev();
            return *this;
        }

        bool operator==(const iterator& rOther) const { return m_pNode == rOther.m_pNode; }
        bool operator!=(const iterator& rOther) const { return !(*this == rOther); }

        IntrusiveListNode* GetNode() const { return m_pNode; }

    private:
        IntrusiveListNode* m_pNode;
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    void push_back(T& rValue) { m_Root.LinkPrev(&NodeTraits::GetNode(rValue)); }
    void push_front(T& rValue) { m_Root.LinkNext(&NodeTraits::GetNode(rValue)); }
    void pop_front() { m_Root.GetNext()->Unlink(); }
    void pop_back() { m_Root.GetPrev()->Unlink(); }

    T& front() { return NodeTraits::GetItem(*m_Root.GetNext()); }
    const T& front() const { return NodeTraits::GetItem(*m_Root.GetNext()); }

    iterator insert(iterator position, T& rValue) {
        IntrusiveListNode* pNode = &NodeTraits::GetNode(rValue);
        position.GetNode()->LinkPrev(pNode);
        return iterator(pNode);
    }

    T& back() { return NodeTraits::GetItem(*m_Root.GetPrev()); }
    const T& back() const { return NodeTraits::GetItem(*m_Root.GetPrev()); }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

    iterator begin() { return iterator(m_Root.GetNext()); }
    const_iterator begin() const { return const_iterator(m_Root.GetNext()); }
    iterator end() { return iterator(&m_Root); }
    const_iterator end() const { return const_iterator(&m_Root); }

    iterator iterator_to(T& rValue) { return iterator(&NodeTraits::GetNode(rValue)); }

    int size() const {
        int count = 0;
        for (auto it = begin(); it != end(); ++it) {
            ++count;
        }
        return count;
    }

    bool empty() const { return !m_Root.IsLinked(); }

    iterator erase(iterator position) {
        if (position == end()) {
            return end();
        }
        iterator temporary(position);
        (temporary++).GetNode()->Unlink();
        return temporary;
    }

private:
    IntrusiveListNode m_Root;
};
};  // namespace util
};  // namespace nn