#include "include/bitmap_buddy.h"
#include "include/assert.h"

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// 向上 2的次幂 对齐
static uint32_t fixsize(unsigned size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size + 1;
}

/*判断bit_idx位是否为1，若为1，则返回true，否则返回false*/
bool bitmap_scan_test(struct bitmap_buddy* btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;   //向下取整用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;   //取余用于索引数组内的位
    return (btmp->bbt[byte_idx] & ((uint32_t)1 << bit_odd));
}

/*将位图btmp的bit_idx位置为value*/
void bitmap_set(struct bitmap_buddy* btmp, uint32_t bit_idx, int8_t value)
{
    assert((value == 0) || (value == 1));
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;

    //一般都会用个0x1这样的数对字节中的位操作
    //将1任意移动后再取反，或者先取反再移位，可用来对位置0操作
    if(value) {
        btmp->bbt[byte_idx] |= ((uint32_t)1 << bit_odd);
    } else {
        btmp->bbt[byte_idx] &= ~((uint32_t)1 << bit_odd);
    }
}



// 给定下标，求结点所在层数，然后可求所在层 内存块大小
static inline uint32_t get_size(uint32_t index)
{
    uint32_t count = 0;
    while (PARENT(index) != 0)
    {
        count++;
    }
    return count+1;
}

struct bitmap_buddy* buddy_new(int size, void *bbase)
{
    struct bitmap_buddy* self;
    uint32_t node_size;
    int i;
    if (size < 1 || !IS_POWER_OF_2(size))
        return NULL;

    self = (struct bitmap_buddy*)bbase; //分配器位置
    self->size = size;
    node_size = 2*size; //管理的二叉树节点是 管理内存单元个数的 2倍

    // 初始化二叉树节点，为所对应的的内存块大小
    // for (i = 0; i < 2 * size - 1; ++i) {
    //     if (IS_POWER_OF_2(i+1))  //树的层次，逐层减一倍
    //         node_size /= 2;
    //     self->bbt[i] = node_size;
    // }
    return self;
}

int buddy_alloc(struct bitmap_buddy* b, int size)
{
    uint32_t index = 0;
    uint32_t node_size;
    uint32_t offset = 0;

    if (b == NULL)
    {
        panic("buddy no initial !\n");
    }
    // 首先将size调整到2的次幂,并检查是否超过最大限度
    if (size <= 0)
        size = 1;
    else if (!IS_POWER_OF_2(size))
        size = fixsize(size);

    // 根节点仅仅表示可分配的最大的块，并不表示所有块总和， 总和为所有上层节点不为0的叶结点的和
    if (b->bbt[0] < size)  
    {
        return -1;
    }
    

    // 然后进行适配搜索，深度优先遍历，当找到对应节点后，将其bbt标记为0
    for(node_size = b->size; node_size != size; node_size /= 2 ) 
    {
        if (b->bbt[LEFT_LEAF(index)] >= size)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }

    // 并转换为内存块索引offset返回
    // 比如内存总体大小32，我们找到节点下标[8]，内存块对应大小是4
    // 则offset = (8+1)*4-32 = 4，那么分配内存块就从索引4开始往后4个单位。
    // 从树层次结构上看就是 第二个大小为4的内存块（7,8）
    b->bbt[index] = 0;
    offset = (index + 1) * node_size - b->size;


    // 回溯，小块分配后 大块也需要分离
    while (index)
    {
        index = PARENT(index);
        b->bbt[index] = MAX(b->bbt[LEFT_LEAF(index)], b->bbt[RIGHT_LEAF(index)]);
    }

    return offset;
}

void buddy_free(struct bitmap_buddy *b, int offset) 
{
    uint32_t node_size, index = 0;
    uint32_t left_longest, right_longest;

    assert(b && offset >= 0 && offset < b->size); //确保有效

    // 反向回溯，从最后的节点开始一直往上找到为0的节点
    // 即当初分配块所适配的大小和位置
    // 将其恢复到原来满状态的值。
    // 继续向上回溯，检查是否存在合并的块，依据就是左右子树的值相加是否等于原空闲块满状态的大小，如果能够合并，就将父节点标记为相加的和
    // 或者左右子树相等
    node_size = 1;
    index = offset + b->size - 1;

    for (; b->bbt[index] ; index = PARENT(index)) 
    {
        node_size *= 2;
        if (index == 0)
            return;
    }

    b->bbt[index] = node_size;

    while (index) 
    {
        index = PARENT(index);
        node_size *= 2;

        left_longest = b->bbt[LEFT_LEAF(index)];
        right_longest = b->bbt[RIGHT_LEAF(index)];

        if (left_longest + right_longest == node_size)
            b->bbt[index] = node_size;
        else
            b->bbt[index] = MAX(left_longest, right_longest);
    }
}


// 返回offset对应内存块大小
int buddy_size(struct bitmap_buddy* self, int offset)
{
    uint32_t node_size, index = 0;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;
    for (index = offset + self->size - 1; self->bbt[index] ; index = PARENT(index))
        node_size *= 2;

    return node_size;
}

// 返回整个buddy剩余的内存块总和
uint64_t buddy_remain_size(struct bitmap_buddy *b)
{
    assert(self);

    uint64_t sum = 0;
    

}



