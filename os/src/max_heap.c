#include "include/max_heap.h"

void heap_init(struct maxHeap* maxheap, uintptr_t base ,const int n) // n ~ MaxSize
{
    maxheapp
}
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