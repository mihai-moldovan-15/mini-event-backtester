#pragma once

#include "Types.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>

/// forward declaration (pentru owner{})
template <typename T>
class List;

template<typename T>
struct ListNode {
    T value{};
    ListNode<T>* prev{};
    ListNode<T>* next{};
    List<T>* owner{};       /// Verificarea ca un nod apartine unei liste
};

///NodePool este o lista simplu inlantuita si next este momentan nefolosit, nodurile alocate in pool
///ajung sa fie folosite in lista dublu inlantuita; daca am defini un nou tip de nod special pentru NodePool,
///am economisi 8 bytes pentru fiecare nod, dar dupa ar trebui facuta o conversie intre nodurile alocate si cele pe care vrem
///sa le folosim
template<typename T>
class NodePool {
private:
    ListNode<T>* m_freeHead{};
    std::vector<std::unique_ptr<ListNode<T>[]>> m_blocks{};
    static const size_t CHUNKSIZE{8192};

public:
    void grow() {
        auto block = std::make_unique<ListNode<T>[]>(CHUNKSIZE);
        ///[] - o specializare a std::make_unique, folosit aici pentru delete[] in loc de delete cand
        ///blockul goes out of scope
        ///[] permite si indexarea

        ///adaugam blockul actual la inceputul listei
        for (size_t i{}; i < CHUNKSIZE - 1; ++i)
            block[i].next = &block[i + 1]; ///legam memoria nou alocata

        block[CHUNKSIZE - 1].next = m_freeHead;
        m_freeHead = &block[0];
        m_blocks.push_back(std::move(block));
    }

    ListNode<T>* allocate() {
        if (!m_freeHead)
            grow();

        ListNode<T>* newNode = m_freeHead;
        m_freeHead = m_freeHead->next;

        newNode->next = nullptr;
        newNode->prev = nullptr;
        return newNode;
    }

    void deallocate(ListNode<T>* node) {
        node->prev = nullptr;
        node->next = m_freeHead;
        m_freeHead = node;
    }
};

template<typename T>
class List {
private:
    ListNode<T>* m_head{};
    ListNode<T>* m_tail{};
    NodePool<T>* m_pool{};
    size_t m_size{};
public:
    List(NodePool<T>* pool) : m_pool(pool) {}
    List (const List&) = delete; ///move only
    List& operator=(const List&) = delete; ///move only
    List(List&& other) noexcept : m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size), m_pool(other.m_pool) {
        other.m_head = other.m_tail = nullptr;
        other.m_pool = nullptr;
        other.m_size = 0;
    }
    List& operator=(List&& other) noexcept {
        if (this == &other)
            return *this;

        m_head = other.m_head;
        m_tail = other.m_tail;
        m_pool = other.m_pool;
        m_size = other.m_size;

        other.m_head = other.m_tail = nullptr;
        other.m_pool = nullptr;
        other.m_size = 0;

        return *this;
    }

    ~List();

    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;

    ListNode<T>* getHead() const { return m_head; }
    ListNode<T>* getTail() const { return m_tail; }

    ListNode<T>* erase(ListNode<T>* node);
    ListNode<T>* insert(ListNode<T>* node, T value);

    void push_back(T value);
    void push_front(T value);

    void pop_back();
    void pop_front();
};

template<typename T>
List<T>::~List() {
    auto *p{ m_head };
    while (m_size)
        p = erase(p);
}

template<typename T>
size_t List<T>::size() const { return m_size; }

template<typename T>
bool List<T>::empty() const { return !m_size; }

template<typename T>
ListNode<T>* List<T>::erase(ListNode<T>* node) {
    if (!node)
        return nullptr;

    if (node->owner != this)
        return nullptr;

    node->owner = nullptr;
    auto* aux = node->next;
    if (node->prev)
        node->prev->next = node->next;
    else
        m_head = node->next;

    if (node->next)
        node->next->prev = node->prev;
    else
        m_tail = node->prev;

    m_pool->deallocate(node);
    -- m_size;

    return aux;
}

template<typename T>
ListNode<T>* List<T>::insert(ListNode<T>* node, T value) {
    if (node && node->owner != this)
        return nullptr;

    if (!node && m_size)
        return nullptr;

    auto* newNode = m_pool->allocate();
    newNode->owner = this;
    newNode->value = std::move(value);
    if (!m_size) {
        m_head = m_tail = newNode;
        ++ m_size;
        return newNode;
    }

    newNode->next = node->next;
    newNode->prev = node;

    if (node->next)
        node->next->prev = newNode;
    else
        m_tail = newNode;

    node->next = newNode;
    ++m_size;
    return newNode;
}

template<typename T>
void List<T>::push_back(T value) {

    if (!insert(m_tail, std::move(value)))
        throw std::logic_error("Tail node does not belong to this list");
}

template<typename T>
void List<T>::push_front(T value) {
    auto* newNode = m_pool->allocate();
    newNode->value = std::move(value);
    newNode->owner = this;

    if (!m_size) {
        m_head = m_tail = newNode;
        ++m_size;
        return;
    }

    newNode->next = m_head;
    newNode->prev = nullptr;
    m_head->prev = newNode;
    m_head = newNode;
    ++m_size;
}

template<typename T>
void List<T>::pop_back() {
    if (!m_size)
        throw std::logic_error("List is empty, UB");

    erase(m_tail);
}

template<typename T>
void List<T>::pop_front() {
    if (!m_size)
        throw std::logic_error("List is empty, UB");

    erase(m_head);
}
