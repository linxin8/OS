#pragma once
#include "lib/stdint.h"

class ListElement
{
public:
    void         remove_from_list();
    ListElement& operator=(const ListElement&) = delete;

public:
    ListElement* previous = nullptr;
    ListElement* next     = nullptr;
};

class List
{
public:
    List();
    List(List&& list);
    List& operator=(List&& list);
    void  init();
    void  insert_before(ListElement* position, ListElement* element);
    void  insert_after(ListElement* position, ListElement* element);
    void  push_front(ListElement* element);
    void  push_front(ListElement& element);
    void  push_back(ListElement* element);
    void  push_back(ListElement& element);
    // void     remove(ListElement* element);
    ListElement* pop_front();
    ListElement* pop_back();
    uint32_t     get_length();
    bool         is_empty();
    bool         find(const ListElement* element);
    bool         find(const ListElement& element);
    ListElement& front();
    ListElement& back();
    void         remove(ListElement* element);
    void         remove(ListElement& element);

    //返回true时停止遍历,失败时返回nullptr
    ListElement* for_each(bool (*pfun)(ListElement* element, void* arg), void* arg);

private:
    ListElement head;
    ListElement tail;
};