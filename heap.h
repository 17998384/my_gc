//
//  heap.h
//  jvm_heap
//
//  jvm堆
//
//  Created by 我叫城北徐公° on 2020/12/15.
//

#ifndef heap_h
#define heap_h
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "j_class_obj.h"


typedef struct heap_eden HeapEden;
typedef struct heap_survivor HeapSurvivor;

typedef struct heap
{
    //堆区使用空间（byte）
    unsigned long heap_used;
    //堆区总大小（byte）
    unsigned long heap_total;
    //堆空间起始地址
    unsigned long heap_start;
    //堆空间结束地址
    unsigned long heap_end;
    //eden区
    HeapEden* heap_eden;
    //from区
    HeapSurvivor* heap_from;
    //to区
    HeapSurvivor* heap_to;
    //操作数栈（暂定只有5个空间）,初始化空间为0
    J_Object* operand_stack[5];
   
}Heap;

struct heap_eden
{
    //eden区起始地址
    unsigned long eden_start;
    //eden区结束地址
    unsigned long eden_end;
    //eden区使用空间（byte）
    unsigned long eden_used;
    //eden区总大小（byte）
    unsigned long eden_total;
};

struct heap_survivor
{
    //survivor区起始地址
    unsigned long survivor_start;
    //survivor区结束地址
    unsigned long survivor_end;
    //survivor区使用空间（byte）
    unsigned long survivor_used;
    //survivor区总大小（byte）
    unsigned long survivor_total;
};

/// 创建堆
/// @param size_mb 堆的大小（MB）
Heap* create_heap(int size_mb);

/// 从堆的eden中申请空间
/// @param heap 堆指针
/// @param instance_klass 类元信息
J_Object* malloc_eden(Heap* heap,Instance_Klass* instance_klass);

/// 从survivor区分配空间（用以复制算法）
/// @param heap 堆
/// @param size_byte 申请大小
unsigned long malloc_survivor(Heap* heap,int size_byte);

/// Minor_gc
/// @param heap 堆指针
void Minor_gc(Heap* heap);

/// 销毁堆
/// @param heap 堆指针
void destory_heap(Heap* heap);

#endif /* heap_h */
