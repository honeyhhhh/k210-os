#include "string.h"
#include "stddef.h"
#include "stdio.h"



struct maxHeap 
{
    uint64_t *heapArray；  //存放数据的数组，当前用于数据类型为物理页号
    int CurrentSize;   //当前堆中元素个数，指向最后一层的最右边的元素的下一个位置
    int MaxSize；   //最大数目
};


void heap_init(struct maxHeap* maxheap, const int n); // n ~ MaxSize
bool heap_empty(struct maxHeap* maxheap);   // 判断是否为空
bool heap_leaf(struct maxHeap* maxheap, int pos);   //判断是否为叶节点
int heap_lc(struct maxHeap* maxheap, int pos);  //返回左孩子位置
int heap_rc(struct maxHeap* maxheap, int pos);  //返回右孩子位置
int heap_parent(struct maxHeap* maxheap, int pos)  //返回父亲位置

bool heap_insert(struct maxHeap* maxheap, const T newnode);
void heap_removeMin(struct maxHeap* maxheap);//从堆顶删除最小值，最小堆找最小值
//互斥操作
void heap_SiftUp(struct maxHeap* maxheap, int pos);//从pos向上开始调整，使序列成为堆//临界条件tempos = 0
void heap_SiftDown(struct maxHeap* maxheap, int pos);//筛选法函数，pos表示开始处理的数组下标
uint64_t heap_top(struct maxHeap* maxheap);


void heap_destory(struct maxHeap* maxheap);
