#include "include/max_heap.h"
#include "include/assert.h"
#include "include/mm.h"

void heap_init(struct maxHeap* maxheap, void *base ,const uint64_t n) // n ~ MaxSize
{
    if (base == NULL)
    {
        //printk("----------%p\n", HEAP_ALLOCATOR);
        base = buddy_alloc(HEAP_ALLOCATOR, 8*n);
    }
    maxheap->heapArray = (uint64_t *)base;
    maxheap->MaxSize = n;
    maxheap->CurrentSize = 0;
}

bool heap_empty(struct maxHeap* maxheap)   // 判断是否为空
{
    return maxheap->CurrentSize == 0;
}
bool heap_leaf(struct maxHeap* maxheap, int pos)   //判断是否为叶节点
{
    return pos > maxheap->CurrentSize/2 && pos < maxheap->CurrentSize;
}
int heap_lc(int pos)  //返回左孩子位置
{
    return 2*pos + 1;
}
int heap_rc(int pos)  //返回右孩子位置
{
    return 2*pos + 2;
}
int heap_parent(int pos)  //返回父亲位置
{
    return (pos - 1)/2;
}

void heap_insert(struct maxHeap* maxheap, const uint64_t newnode)
{
    if (maxheap->CurrentSize == maxheap->MaxSize)
    {
        panic("no space for maxheap !\n");
    }
    maxheap->heapArray[maxheap->CurrentSize] = newnode;
    heap_SiftUp(maxheap, maxheap->CurrentSize);
    maxheap->CurrentSize++;
}
void heap_removeMax(struct maxHeap* maxheap)
{
    if (heap_empty(maxheap))
    {
        panic("no elements in this maxheap !\n");
    }
    uint64_t t = maxheap->heapArray[0];
    maxheap->heapArray[0] =  maxheap->heapArray[maxheap->CurrentSize - 1];
    maxheap->heapArray[maxheap->CurrentSize - 1] = t;
    maxheap->CurrentSize--;

    if (maxheap->CurrentSize > 1)
    {
        heap_SiftDown(maxheap,0);
    }
}
//互斥操作
void heap_SiftUp(struct maxHeap* maxheap, int pos)//从pos向上开始调整，使序列成为堆//临界条件tempos = 0
{
    int temppos = pos;
    uint64_t temp = maxheap->heapArray[temppos];
    while (temppos > 0 && maxheap->heapArray[heap_parent(temppos)] < temp)
    {
        maxheap->heapArray[temppos] = maxheap->heapArray[heap_parent(temppos)];
        temppos = heap_parent(temppos);
    }
    maxheap->heapArray[temppos] = temp;
}
void heap_SiftDown(struct maxHeap* maxheap, int pos)//筛选法函数，pos表示开始处理的数组下标
{
    int i = pos;//标识父结点
    int j = 2*i+1;//表示关键值较小的子结点//左孩子
    uint64_t temp = maxheap->heapArray[i];//保存父结点
    
    while (j < maxheap->CurrentSize)
    {
        if (j < maxheap->CurrentSize-1 && maxheap->heapArray[j] < maxheap->heapArray[j+1]) {
            j++;//j指向较小的子结点
        }
        if (temp < maxheap->heapArray[j]) {
            maxheap->heapArray[i] = maxheap->heapArray[j];
            i = j;
            j = 2*j+1;//向下继续
        }
        else
            break;
    }
    maxheap->heapArray[i] = temp;//不是向下交换，而是把下面元素往上推
}
uint64_t heap_top(struct maxHeap* maxheap)
{
    return maxheap->heapArray[0];
}


void heap_destory(struct maxHeap* maxheap)
{

}

bool is_some(struct maxHeap *m, uint64_t ppn)
{
    for (int i = 0; i < m->CurrentSize; i++)
    {
        if (m->heapArray[i] == ppn)
            return 1;
    }
    return 0;
}