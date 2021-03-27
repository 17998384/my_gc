//
//  j_object.h
//  jvm_heap
//
//  jvm对象
//
//  Created by 我叫城北徐公° on 2020/12/15.
//

#ifndef j_class_obj_h
#define j_class_obj_h

typedef char j_byte;
typedef struct instance_klass Instance_Klass;
typedef struct j_object J_Object;

//类元信息
struct instance_klass
{
    char* class_name;          //类名
    int instance_klass_size;    //类模版大小
    int field_num;             //字段数量
    int* field_offset;         //字段偏移量
};

//jvm对象
struct j_object
{
    //对象头
    Instance_Klass* instence_klass;    //指向其类元信息
    int forword;                     //该类是否已经复制过
    J_Object* forwording;             //复制后的新地址
    //对象体不表示在此，但空间会根据类模版的大小进行分配
};

#endif /* j_class_obj_h */
