//
//  main.c
//  jvm_heap
//
//  Created by 我叫城北徐公° on 2020/12/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "heap.h"

typedef struct test1_class Test1Class;
typedef struct test2_class Test2Class;
typedef struct test3_class Test3Class;

//类模版
//模版1
struct test1_class
{
    //对象头
    Instance_Klass* instence_klass;    //指向其类元信息
    int forword;                     //该类是否已经复制过
    J_Object* forwording;             //复制后的新地址
    Test2Class* t2;
};

//模版2
struct test2_class
{
    //对象头
    Instance_Klass* instence_klass;    //指向其类元信息
    int forword;                     //该类是否已经复制过
    J_Object* forwording;             //复制后的新地址
    Test1Class* t1;
};

//模版3
struct test3_class
{
    //对象头
    Instance_Klass* instence_klass;    //指向其类元信息
    int forword;                     //该类是否已经复制过
    J_Object* forwording;             //复制后的新地址
    Test1Class* t1;
    Test2Class* t2;
};

//类元信息1
Instance_Klass test1 =
{
    "test",
    sizeof(Test1Class),
    1,
    (int []){offsetof(Test1Class, t2)}
};

//类元信息2
Instance_Klass test2 =
{
    "test1",
    sizeof(Test2Class),
    1,
    (int []){offsetof(Test2Class, t1)}
};

//类元信息3
Instance_Klass test3 =
{
    "test1",
    sizeof(Test3Class),
    2,
    (int []){offsetof(Test3Class, t1),offsetof(Test3Class, t2)}
};

int main(void)
{
    //2M的堆空间
    Heap* heap = create_heap(2);
    //演示循环依赖问题
    Test1Class* t1 = (Test1Class*)malloc_eden(heap, &test1);
    Test2Class* t2 = (Test2Class*)malloc_eden(heap, &test2);
//    Test3Class* t3 = (Test3Class*)malloc_eden(heap, &test3);
//    printf("t1 addr : %p,t2 addr : %p,t3 : %p\n",t1,t2,t3);
    heap->operand_stack[0] = (J_Object*)t1;
    //Object a = t1;
    //t1与t2循环依赖，并且t1对象被操作数栈0所引用，所以t1,t2均不会回收
    t1->t2 = t2;
    t2->t1 = t1;
    heap->operand_stack[0] = NULL;
    //Object a = null;
//    //t3未被引用，所以会被回收
//    t3->t1 = t1;
//    t3->t2 = t2;
//    heap->operand_stack[1] = (J_Object*)t3;
//    heap->operand_stack[0] = NULL;
//    heap->operand_stack[1] = NULL;
    Minor_gc(heap);
//    printf("t1 addr : %p,t2 addr : %p,t3 : %p\n",heap->operand_stack[0],((Test1Class*)heap->operand_stack[0])->t2,NULL);
    destory_heap(heap);

    return 0;
}
