#include "include/bitmap_buddy.h"
#include "include/assert.h"
#include "include/string.h"
#include "include/stdio.h"

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

static inline uint8_t log2(uint32_t value)   //判断一个数是2的多少次方  
{  

    uint8_t x = 0;  
    while(value > 1)  
    {  
        value >>= 1;  
        x++;  
    }  
    return x;  
}
static inline uint64_t pow2(uint8_t b)
{
    uint64_t a = 2;
    uint64_t res = 1;
    while (b > 0) {
        if (b & 1) res = res * a;
        a = a * a;
        b >>= 1;
    }
    return res;
}

/*判断bit_idx位是否为1，若为1，则返回true，否则返回false 注意true并不是1*/
static inline bool bitmap_scan_test(struct bitmap_buddy* btmp, uint32_t bit_idx)
{
    uint32_t base = btmp->size/16 - 1;
    uint32_t byte_idx = (bit_idx - base) / 8;   //向下取整用于索引数组下标
    uint32_t bit_odd = bit_idx % 8;   //取余用于索引数组内的位
    

    return (btmp->bbt[base + byte_idx] & ((uint32_t)1 << bit_odd));
}

/*将位图btmp的bit_idx位置为value*/
void bitmap_set(struct bitmap_buddy* btmp, uint32_t bit_idx, int8_t value)
{
    assert((value == 0) || (value == 1));
    uint32_t base = btmp->size/16 - 1;

    uint32_t byte_idx = (bit_idx - base) / 8;
    uint32_t bit_odd = bit_idx % 8;

    //一般都会用个0x1这样的数对字节中的位操作
    //将1任意移动后再取反，或者先取反再移位，可用来对位置0操作
    if(value) {
        btmp->bbt[base + byte_idx] |= ((uint32_t)1 << bit_odd);
    } else {
        btmp->bbt[base + byte_idx] &= ~((uint32_t)1 << bit_odd);
    }
}

static inline bool in_bitmap(struct bitmap_buddy *b ,uint32_t index)
{
    return index >= b->size/16 - 1;
}

// 给定层 下标 求 内存块大小
static uint32_t get_size(struct bitmap_buddy *b, uint32_t index)
{
    uint32_t size = 0;
    if (in_bitmap(b,index))
    {
        if (in_bitmap(b, PARENT(index)))  // 8字节层
        {
            if (bitmap_scan_test(b, index))
                size = log2(8);
        }
        else    // 16字节层
        {
            if (bitmap_scan_test(b, index))  //为0
            {
                bool l = bitmap_scan_test(b, LEFT_LEAF(index));
                bool r = bitmap_scan_test(b, RIGHT_LEAF(index));
                if (l&&r)
                    size = log2(16);
                else if (l||r)
                    size = log2(8);
                else
                    size = 0;
            }
        }
    }
    else
    {
        size = b->bbt[index];
    }
    return size;
    
}

struct bitmap_buddy* buddy_new(int size, void *bbase)
{
    struct bitmap_buddy* self;
    uint32_t node_size;
    int i;
    if (size < 1 || !IS_POWER_OF_2(size))
        return NULL;

    self = (struct bitmap_buddy*)bbase; //分配器位置
    self->size = size; //初始最大块，也就是初始时内存块总和
    self->min_alloc_size = 8;
    self->space = NULL;
    node_size = 2*size; 
    initlock(&self->lock, "kheap");
    //管理的二叉树节点是 管理内存单元个数的 2倍 - 1

    //初始化二叉树节点，为所对应的的内存块大小
    for (i = 0; i < 2 * size - 1; ++i) 
    {
        if (IS_POWER_OF_2(i+1))  //树的层次，逐层减一倍
        {
            node_size /= 2;
            //printk("&bbt[%d]:[%p]-[%d] \n",i, &self->bbt[i], node_size);

            if (node_size == 16)
                break;
        }
        self->bbt[i] = log2(node_size);
    }
    //printk("&bbt[%d]:[%p]-[%d] \n",i-1, &self->bbt[i-1], self->bbt[i-1]);
    //printk("&bbt[%d]:[%p]-[%d] \n",4094, &self->bbt[4094], self->bbt[4094]);

    //printk("&bbt[%d]:[%p]-[%d] \n",4095, &self->bbt[4095], self->bbt[4095]);

    //printk("&bbt[%d]:[%p]-[%d] \n",i, &self->bbt[i], self->bbt[i]);


    // 假设管理16字节，则结点数 2*16 - 1 = 31个，即31位 = 3字节+7位
    // 因为分配最小8字节，所以结点数16/4 - 1 = 3位 
    // 实际节点数
    uint32_t bit_node_size = size/self->min_alloc_size + size/(self->min_alloc_size*2);

    //printk("max node size : [%d]k\n", size/1024); //128k 2^17 0x20000H
    //printk("bit nums: [%d]k bits\n", bit_node_size/1024); //
    memset(&self->bbt[i], UCHAR_MAX, bit_node_size/8);
    //printk("base %p of %d k byte's bit set 1\n", &self->bbt[i], bit_node_size/8/1024);
    //printk("b->size/16 - 1:[%d]\n", self->size/16-1);
 
    return self;
}

void *buddy_alloc(struct bitmap_buddy* b, uint32_t size)
{
    acquire(&b->lock);
    uint32_t index = 0;
    uint32_t node_size;
    uint64_t offset = 0;

    if (b == NULL)
    {
        printk("buddy no initial !\n");
        return NULL;
    }
    // 首先将size调整到2的次幂,并检查是否超过最大限度
    if (size <= 8)
        size = 8;
    else if (!IS_POWER_OF_2(size))
        size = fixsize(size);

    // 根节点仅仅表示可分配的最大的块，并不表示所有块总和， 总和需要计算已分配的空间
    //printk("max: %d filesize :%d\n", b->bbt[0],size);
    if (pow2(b->bbt[0]) < size)  
    {
        printk("no space !\n");
        return NULL;
    }
    //printk("alloc %d byte\n", size);
    

    // 然后进行适配搜索，深度优先遍历,二叉搜索 ，当找到对应节点后，将其bbt标记为0
    // size 一定是2的次幂且 根节点一定大于size

    // 当index为最后两层范围时需要使用bitmap
    // 16层 的结点值为1 时实际值有可能是8或16

    for(node_size = b->size; node_size != size; node_size /= 2 ) 
    {

        if (pow2(get_size(b, LEFT_LEAF(index))) >= size)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);        
    }
    //printk("node_size :%d  index :%d\n", node_size, index);

    


    // 并转换为内存块索引offset返回
    // 比如内存总体大小32，我们找到节点下标[8]，内存块对应大小是4
    // 则offset = (8+1)*4-32 = 4，那么分配内存块就从索引4开始往后4个单位。
    // 从树层次结构上看就是 第二个大小为4的内存块（7,8）
    if (size <= 16)
        bitmap_set(b, index, 0);
    else
        b->bbt[index] = 0;
    offset = (index + 1) * node_size - b->size;
    //printk("alloc size :[%d],space [%p] index [%d] off:[%p] ptr:[%p]\n", size,b->space, index,offset ,((uint64_t)b->space +  offset));


    // 回溯，小块分配后 大块也需要分离，标记为左右子树最大值
    while (index)
    {
        if (in_bitmap(b, index))
        {
            index = PARENT(index);
            if (in_bitmap(b, index))
            {
                uint8_t left = bitmap_scan_test(b, LEFT_LEAF(index)) > 0 ? 1:0;
                uint8_t right = bitmap_scan_test(b, RIGHT_LEAF(index)) > 0 ? 1:0;
                //printk("%d:%d %d:%d\n",LEFT_LEAF(index) ,left, RIGHT_LEAF(index),right);
                if (left|right)
                    bitmap_set(b, index, 1);
                else
                    bitmap_set(b, index, 0);
            }
            else
            {
                //printk("%d: %d %d : %d\n", LEFT_LEAF(index),get_size(b,LEFT_LEAF(index)), RIGHT_LEAF(index),get_size(b,RIGHT_LEAF(index)));
                b->bbt[index] = MAX(get_size(b,LEFT_LEAF(index)), get_size(b,RIGHT_LEAF(index)));
                //printk("index %d: %d \n",index, b->bbt[index]);
            }
        }
        else
        {
            index = PARENT(index);
            b->bbt[index] = MAX(b->bbt[LEFT_LEAF(index)], b->bbt[RIGHT_LEAF(index)]);
        }
    }
    memset((void *)((uint64_t)b->space +  offset), 0, size);
    release(&b->lock);
    return (void *)((uint64_t)b->space +  offset); //?
}

void buddy_free(struct bitmap_buddy *b, void *ptr) 
{
    if (ptr == NULL)
    {
        return ;
    }
    acquire(&b->lock);
    uint64_t offset = (uint64_t)ptr - (uint64_t)b->space;
    uint32_t node_size, index = 0;
    uint32_t left_longest, right_longest;

    assert(b && offset >= 0 && offset < b->size); //确保有效

    // 反向回溯，从最后的节点开始一直往上找到为0的节点?  第一个为0的一定是，每一层都有自己的大小
    // 即当初分配块所适配的大小和位置
    // 将其恢复到原来满状态的值。
    // 继续向上回溯，检查是否存在合并的块，依据就是左右子树的值相加是否等于原空闲块满状态的大小，如果能够合并，就将父节点标记为相加的和
    // 或者左右子树相等
    node_size = 1;
    index = offset + b->size - 1;
    while (node_size != 8)
    {
        index = PARENT(index);
        node_size*=2;
    }

    for (; get_size(b, index) ; index = PARENT(index))
    {
        node_size *= 2;
        if (index == 0)
            panic("unknow fault in free \n");
    }
    if (in_bitmap(b, index))
        bitmap_set(b, index, 1);
    else
        b->bbt[index] = log2(node_size);
    
    //printk("free ptr [%p] offset[%p] index [%d] size: %d\n", ptr, offset, index ,node_size);
    

    // [index] 原本是0，已经设置成原来的大小
    while (index) 
    {

        index = PARENT(index);
        node_size *= 2;
        //printk("init index %d size: %d\n", index, get_size(b, index));

        left_longest = get_size(b, LEFT_LEAF(index));
        right_longest = get_size(b, RIGHT_LEAF(index));

        if (in_bitmap(b, index))
        {
            bitmap_set(b, index, 1);
        }
        else
        {
            if (left_longest + right_longest == log2(node_size))
                b->bbt[index] = log2(node_size);
            else
                b->bbt[index] = MAX(left_longest, right_longest);
        }

    }
    //printk("free index %p size: %d\n", ptr, pow2(get_size(b, index)));

    release(&b->lock);

    ptr = NULL;

}


// 返回offset对应内存块大小
int buddy_size(struct bitmap_buddy* self, uint64_t offset)
{
    offset = offset - (uint64_t)self->space;
    uint64_t node_size, index = 0;

    assert(self && offset >= 0 && offset < self->size);
    
    // offset = (index + 1) * node_size - b->size;
    // offset + self->size - 1 表示offset对应元素的最左叶节点，并且内存块大小为1，一一对应
    node_size = 1;
    index = offset + self->size - 1;
    while (node_size != 8)
    {
        index = PARENT(index);
        node_size*=2;
    }


    for (; get_size(self, index) ; index = PARENT(index))
        node_size *= 2;

    return node_size;
}

static inline bool idx_illegal(struct bitmap_buddy *b, uint32_t index)
{
    return index < b->size/4 - 1;
}
static inline uint64_t get_size2(struct bitmap_buddy *b, uint32_t idx)
{
    uint64_t node_size = b->size;
    while (idx)
    {
        node_size/=2;
        idx = PARENT(idx);
    }
    return node_size;
}

// 返回整个buddy剩余的内存块总和
// 在算法稳定后可以维护一个变量用来记录 已分配的控件
uint64_t buddy_remain_size(struct bitmap_buddy *b)
{
    assert(b);
    uint64_t alloced = 0;
    uint32_t index = 0;

    // 深度遍历   累加每条路径上第一个0值结点对应的值
    // 当一个结点为0，那么的父亲的值要么为它的兄弟结点，要么为0
    // 一个结点的值仅仅表示其管理区域可分配空间的最大值
    uint32_t astack[50];
    astack[0] = -1;
    int sp = 1;

    while (index != -1)
    {
        if (get_size(b, index) == 0)
        {
            alloced += get_size2(b, index);
            index = astack[sp-1];
            sp-=1;
            continue;
        }

        if (idx_illegal(b, RIGHT_LEAF(index)))
        {
            astack[sp] = RIGHT_LEAF(index);
            sp++;
        }

        
        if (idx_illegal(b, LEFT_LEAF(index)))
        {
            index = LEFT_LEAF(index);
        }
        else
        {
            index = astack[sp-1];
            sp-=1;
        }
        
    }
    
    return b->size - alloced;
}



