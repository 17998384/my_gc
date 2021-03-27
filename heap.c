//
//  heap.c
//  jvm_heap
//
//  Created by 我叫城北徐公° on 2020/12/15.
//

#include "heap.h"

/// 创建堆
/// @param size_mb 堆的大小（MB）
Heap* create_heap(int size_mb)
{
    if (size_mb % 2 != 0 || size_mb <= 0)
    {
        printf("堆大小只支持2的整数倍，并且不能低于0mb！\n");
        exit(1);
    }
    //创建堆结构体
    //申请堆空间
    Heap* heap = (Heap*)calloc(1, sizeof(Heap));
    heap->heap_total = size_mb * 1024 * 1024;
    heap->heap_used = 0;
    heap->heap_start = (unsigned long)calloc(heap->heap_total,1);
    heap->heap_end = heap->heap_start + heap->heap_total - 1;
    //堆中eden ：s0 : s1 -> 2 : 1 : 1 （为方便演示回收过程）
    //eden
    heap->heap_eden = (HeapEden*)malloc(sizeof(HeapEden));
    HeapEden* heap_eden = heap->heap_eden;
    heap_eden->eden_used = 0;
    heap_eden->eden_total = (heap->heap_total / 4) * 2;
    heap_eden->eden_start = heap->heap_start;
    heap_eden->eden_end = heap_eden->eden_start + heap_eden->eden_total - 1;
    //s0
    heap->heap_from = (HeapSurvivor*)malloc(sizeof(HeapSurvivor));
    HeapSurvivor* heap_from = heap->heap_from;
    heap_from->survivor_used = 0;
    heap_from->survivor_total = (heap->heap_total / 4);
    heap_from->survivor_start = heap_eden->eden_end + 1;
    heap_from->survivor_end = heap_from->survivor_start + heap_from->survivor_total - 1;
    //s1
    heap->heap_to = (HeapSurvivor*)malloc(sizeof(HeapSurvivor));
    HeapSurvivor* heap_to= heap->heap_to;
    heap_to->survivor_used = 0;
    heap_to->survivor_total = (heap->heap_total / 4);
    heap_to->survivor_start = heap_from->survivor_end + 1;
    heap_to->survivor_end = heap_to->survivor_start + heap_to->survivor_total - 1;
    printf("[heap young size -> %ldkb,%p,%p heap eden size -> %ldkb,%p,%p heap from size -> %ldkb,%p,%p heap to size -> %ldkb,%p,%p]\n",heap->heap_total / 1024,heap->heap_start,heap->heap_end,heap_eden->eden_total / 1024,heap_eden->eden_start,heap_eden->eden_end,heap_from->survivor_total / 1024,heap_from->survivor_start,heap_from->survivor_end,heap_to->survivor_total / 1024,heap_to->survivor_start,heap_to->survivor_end);
    printf("[the heap create success!]\n");
    return heap;
}

/// 从堆的eden中申请空间
/// @param heap 堆指针
/// @param instance_klass 类元信息
J_Object* malloc_eden(Heap* heap,Instance_Klass* instance_klass)
{
    if (instance_klass == NULL) return NULL;
    int count = 0;
    while (count < 2)
    {
        //先从eden区进行分配空间
        unsigned long eden_available_space = heap->heap_eden->eden_total - heap->heap_eden->eden_used;
        if (eden_available_space >= instance_klass->instance_klass_size)
        {
            //eden区使用量增加
            unsigned long res_addr = heap->heap_eden->eden_start + heap->heap_eden->eden_used;
            //堆总内存使用量增加
            heap->heap_used += instance_klass->instance_klass_size;
            //满足条件，则从eden区分配空间
            heap->heap_eden->eden_used += instance_klass->instance_klass_size;
            //初始化申请的空间
            memset((void*)res_addr,0,instance_klass->instance_klass_size);
            J_Object* j_object = (J_Object*)res_addr;
            j_object->forword = FALSE;
            j_object->forwording = NULL;
            j_object->instence_klass = instance_klass;
            return j_object;
        }
        //eden区空间不足，先触发一次gc
        Minor_gc(heap);
        //数量加1
        count++;
    }
    //内存不足，直接退出程序
    destory_heap(heap);
    printf("out of memory!\n");
    exit(3);
    return 0;
}

/// 从survivor区分配空间（用以复制算法）
/// @param heap 堆
/// @param size_byte 申请大小
unsigned long malloc_survivor(Heap* heap,int size_byte)
{
    //永远从to区进行申请空间以用作复制算法
    if (heap->heap_to->survivor_total - heap->heap_to->survivor_used < size_byte)
    {
        printf("复制算法所需空间不足，需要移动到老年代，但该jvm未设计老年代，故退出jvm\n");
    exit(3);
    }
    //申请到的内存地址
    unsigned long res_addr = heap->heap_to->survivor_start + heap->heap_to->survivor_used;
    //to区使用量增加
    heap->heap_to->survivor_used += size_byte;
//    //堆总内存使用量增加(从s0或s1申请不增加堆内存）
//    heap->heap_used += size_byte;
    return res_addr;
}

/// bfs gc
/// @param heap 堆
/// @param _j_object 对象地址
void BFS_GC(Heap* heap,J_Object** _j_object)
{
//    如果只有eden区使用，s0未使用，则eden区拷贝到s0区 当Eden满时，会触发MinorGC算法来回收memory，旨在清理掉再无引用的数据（在内存里是Tree），意图存储到S0. 若此时S0也满了，会再次MinorGC意图回收S0无引用的数据，把有引用的数据移动到S1。如果S1够用，此时会清空S0；如果S1满了，会回滚刚存入S1的数据，直接把本次GC的数据存入Old区，S0保持刚刚MinorGC时的状态。延伸：如果Old也满了，会触发MajorGC，如果还是不够，则存入Permanent Generate，不幸这里也满了，会在允许的范围内按照内置的规则自动增长，可能不会发生GC，也可能会。当增长的量不够存时，会触发Full GC。若FullGC后还是不够存，自动增长的量也超过了允许的范围，则发生内存溢出。还有一种情况，就是分配的线程栈处于很深的递归或死循环时，会发生栈内存溢出。所以从这里可以看出GC的频率是 Eden > S0 > S1 > Old > Permanent，咱们在优化时尽量让GC发生在Eden，避免大对象直接跨过S0， S1,而直接到Old了。另外，每次GC JVM都会停，GC后都会动态调整各区的大小，如果S1老是不来，常常会分配很小的内存，甚至是个位数。
    
    //年轻代采用复制算法
    //复制算法需要获取的是哪些是有用的，也就是说可达性分析的过程中已经完成了筛选，分析过程中就可以将这一部分内容复制到另一半空间中，然后把原来的一半空间完全清除就可以了，没有标记的必要。
    J_Object* j_object = (*_j_object);
    J_Object* new_obj;
    if (j_object == NULL) return;
    //如果roots节点指向的对象已经复制完了，则直接更新roots节点的内存大小即可，不需要重新分配空间
    if (j_object->forword == TRUE) *(_j_object) = j_object->forwording;
    //否则需要复制一次
    else
    {
        //为roots节点指向的第一个对象分配空间
        J_Object* new_obj = (J_Object*)malloc_survivor(heap, (j_object)->instence_klass->instance_klass_size);
        memcpy((void*)new_obj, (void*)j_object, j_object->instence_klass->instance_klass_size);
        new_obj->forword = FALSE;
        new_obj->forwording = NULL;
        //roots指针更新地址
        (*_j_object) = new_obj;
        //源对象也要更新成已标记
        j_object->forword = TRUE;
        j_object->forwording = new_obj;
    }
    
    /*
        队列与bfs区
     */
    //队列
    J_Object* list[1000] = {0};
    //队列前后指针
    int before_p = 0,after_p = 0;
    //将roots指向的第一个对象，将其所有引用放入队列
    for (int i = 0,size = j_object->instence_klass->field_num; i < size; i++)
    {
        //依次遍历其每个字段指向的引用是否存在新地址
        unsigned long addr = (unsigned long)((void*)j_object + j_object->instence_klass->field_offset[i]);
        J_Object* ref_obj = *((J_Object**)addr);
        //检查是否不为NULL，放入队列
        if (ref_obj != NULL) list[after_p++] = ref_obj;
    }
    //bfs
    while(before_p < after_p)
    {
        //弹出对象
        j_object = list[before_p++];
        if (j_object->forword == TRUE) continue;
        //申请空间
        new_obj = (J_Object*)malloc_survivor(heap, j_object->instence_klass->instance_klass_size);
        //拷贝地址
        memcpy((void*)new_obj, j_object, j_object->instence_klass->instance_klass_size);
        j_object->forword = TRUE;
        j_object->forwording = new_obj;
        for (int i = 0,size = j_object->instence_klass->field_num; i < size; i++)
        {
            //依次遍历其每个字段指向的引用是否存在新地址
            unsigned long addr = (unsigned long)((void*)(j_object) + (j_object)->instence_klass->field_offset[i]);
            J_Object* ref_obj = *((J_Object**)addr);
            //检查是否不为NULL，放入队列
            if (ref_obj != NULL) list[after_p++] = ref_obj;
        }
    }
}

/// 更新to区地址
/// @param heap 堆
void update_heap_to_addr(Heap* heap)
{
    for (unsigned long s = heap->heap_to->survivor_start,size = s + heap->heap_to->survivor_used; s < size;)
    {
        J_Object* j_object = (J_Object*)s;
        //找到对象的指向地址，进行更新
        for (int i = 0,size = j_object->instence_klass->field_num; i < size; i++)
        {
            //依次遍历其每个字段指向的引用是否存在新地址
            unsigned long addr = (unsigned long)((void*)j_object + j_object->instence_klass->field_offset[i]);
            J_Object* ref_obj = *(J_Object**)addr;
            //检查是否更新地址，有则更新
            if (ref_obj != NULL && ref_obj->forwording != NULL) (*(unsigned long*)addr) = (unsigned long*)ref_obj->forwording;
            //将该对象的转发指针置空
            j_object->forword = FALSE;
            j_object->forwording = NULL;
        }
        s += j_object->instence_klass->instance_klass_size;
    }
}

/// Minor_gc
/// @param heap 堆指针
void Minor_gc(Heap* heap)
{
    //可达性分析法，标记正在使用的对象  将每个栈帧的操作数栈当前rootRef节点，进行bfs并复制，此处为了简化，只用数组模拟操作数栈
    printf("[Minor GC begin ..]\n");
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long begin_time = ts.tv_nsec;
    for (int i = 0; i < 5; i++) BFS_GC(heap,&heap->operand_stack[i]);
    //更新to区地址
    update_heap_to_addr(heap);
    printf("[heap to used : %ldbyte]",heap->heap_to->survivor_used);
    //整体gc后，清空eden区和from区
    heap->heap_eden->eden_used = 0;
    heap->heap_from->survivor_used = 0;
    //之前的堆使用
    unsigned long before_heap_used = heap->heap_used;
    //现在的堆使用
    heap->heap_used = heap->heap_to->survivor_used;
    //将from区和to区进行调换
    HeapSurvivor* tmp = heap->heap_from;
    heap->heap_from = heap->heap_to;
    heap->heap_to = tmp;
    //结束时间
    clock_gettime(CLOCK_REALTIME, &ts);
    long end_time = ts.tv_nsec;
    printf("[Minor GC [young : %ldbyte -> %ldbyte(%ldbyte) eden : %ldbyte used ,s0 : %ldbyte used ,s1 : %ldbyte used , %d nano second]\n",before_heap_used,heap->heap_used,heap->heap_total,heap->heap_eden->eden_used, heap->heap_from->survivor_used,heap->heap_to->survivor_used,(int)(end_time - begin_time));
}

/// 销毁堆
/// @param heap 堆指针
void destory_heap(Heap* heap)
{
    if (heap == NULL) return;
    free((void*)heap->heap_start);
    printf("[destory heap %ldkb -> %dkb]\n",heap->heap_total / 1024,0);
    free(heap);
    printf("the heap is destoryed!\n");
}
