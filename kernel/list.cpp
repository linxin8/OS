#include "kernel/list.h"
#include "kernel/asm_interface.h"
#include "kernel/interrupt.h"
#include "lib/debug.h"
#include "thread/sync.h"

void ListElement::remove_from_list()
{
    ASSERT(previous != nullptr);
    ASSERT(next != nullptr);
    previous->next = next;
    next->previous = previous;
    previous       = nullptr;
    next           = nullptr;
}

List::List()
{
    head.previous = nullptr;
    head.next     = &tail;
    tail.previous = &head;
    tail.next     = nullptr;
}

void List::insert_before(ListElement* position, ListElement* element)
{
    //由于数据结构的缺陷，防止插入另一个链表的节点
    ASSERT(element->previous == nullptr);
    ASSERT(element->next == nullptr);
    position->previous->next = element;
    element->previous        = position->previous;
    element->next            = position;
    position->previous       = element;
}

void List::insert_after(ListElement* position, ListElement* element)
{
    //由于数据结构的缺陷，防止插入另一个链表的节点
    ASSERT(element->previous == nullptr);
    ASSERT(element->next == nullptr);
    position->next->previous = element;
    element->next            = position->next;
    element->previous        = position;
    position->next           = element;
}

void List::init()
{
    head.previous = nullptr;
    head.next     = &tail;
    tail.previous = &head;
    tail.next     = nullptr;
}

void List::push_front(ListElement* element)
{
    insert_after(&head, element);
}
void List::push_front(ListElement& element)
{
    push_front(&element);
}
void List::push_back(ListElement* element)
{
    insert_before(&tail, element);
}
void List::push_back(ListElement& element)
{
    push_back(&element);
}
ListElement* List::pop_front()
{
    ASSERT(!is_empty());
    auto element            = head.next;
    head.next               = element->next;
    element->next->previous = &head;
    element->previous       = nullptr;
    element->next           = nullptr;
    return element;
}

ListElement& List::front()
{
    ASSERT(!is_empty());
    return *head.next;
}

ListElement& List::back()
{
    ASSERT(!is_empty());
    return *tail.previous;
}

ListElement* List::pop_back()
{
    ASSERT(!is_empty());
    auto element            = tail.previous;
    tail.previous           = element->previous;
    element->previous->next = &tail;
    element->previous       = nullptr;
    element->next           = nullptr;
    return element;
}

uint32_t List::get_length()
{
    uint32_t count = 0;
    for (auto p = head.next; p != &tail; p = p->next)
    {
        count++;
    }
    return count;
}
bool List::is_empty()
{
    return head.next == &tail;
}

bool List::find(const ListElement* element)
{
    ASSERT(element != nullptr);
    for (auto it = head.next; it != &tail; it = it->next)
    {
        if (it == element)
        {
            return true;
        }
    }
    return false;
}
bool List::find(const ListElement& element)
{
    return find(&element);
}

void List::remove(ListElement* element)
{
    ASSERT(element != nullptr);
    for (auto it = head.next; it != &tail; it = it->next)
    {
        if (it == element)
        {
            it->remove_from_list();
            return;
        }
    }
    ASSERT(false);  //找不到元素，异常
}
void List::remove(ListElement& element)
{
    return remove(&element);
}

List::List(List&& r)
{
    *this = (List &&)(r);
}

List& List::operator=(List&& r)
{
    if (this == &r)
    {
        return *this;
    }
    head.previous = nullptr;
    tail.next     = nullptr;
    if (r.get_length() == 0)
    {
        head.next     = &tail;
        tail.previous = &head;
    }
    else
    {
        head.next           = r.head.next;
        tail.previous       = r.tail.previous;
        head.next->previous = &head;
        tail.previous->next = &tail;
    }
    r.head.next     = &r.tail;
    r.tail.previous = &r.head;
    return *this;
}

ListElement* List::for_each(bool (*pfun)(ListElement* element, void* arg), void* arg)
{
    ListElement* p = head.next;
    while (p != &tail)
    {
        if (pfun(p, arg))
        {
            return p;
        }
        p = p->next;
    }
    return nullptr;
}