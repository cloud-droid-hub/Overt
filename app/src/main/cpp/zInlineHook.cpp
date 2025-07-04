#include "zInlineHook.h"
#define PAGE_START(x)  ((x) & PAGE_MASK)

// 寄存器选择:
// x0~x7:传递子程序的参数和返回值，使用时不需要保存，多余的参数用堆栈传递，64位的返回结果保存在x0中。
// x8:用于保存子程序的返回地址，使用时不需要保存。
// x9~x15:临时寄存器，也叫可变寄存器，子程序使用时不需要保存。
// x16~x17:子程序内部调用寄存器（IPx），使用时不需要保存，尽量不要使用。
// x18:平台寄存器，它的使用与平台相关，尽量不要使用。
// x19~x28:临时寄存器，子程序使用时必须保存。
// x29:帧指针寄存器（FP），用于连接栈帧，使用时必须保存。
// x30:链接寄存器（LR)，用于保存子程序的返回地址。
// x31:堆栈指针寄存器（SP)，用于指向每个函数的栈顶。
// 标志寄存器
// mrs x15,NZCV msr NZCV,x15
// 普通寄存器 STP   x29,x30,[SP,#Ox60+var_so]
// LDP x29,x30,[SP,#Ox60+var_so]
// 申请栈空间中间的代码不能用到sp (sprintf用到了sp)

#define PUSH_ALL()                      \
    asm("sub SP, SP, #0x200");          \
    asm("mrs x18, NZCV");               \
    asm("stp X29, X30, [SP,#0x10]");    \
    asm("stp X0, X1, [SP,#0x20]");      \
    asm("stp X2, X3, [SP,#0x30]");      \
    asm("stp X4, X5, [SP,#0x40]");      \
    asm("stp X6, X7, [SP,#0x50]");      \
    asm("stp X8, X9, [SP,#0x60]");      \
    asm("stp X10, X11, [SP,#0x70]");    \
    asm("stp X12, X13, [SP,#0x80]");    \
    asm("stp X14, X15, [SP,#0x90]");    \
    asm("stp X16, X17, [SP,#0x100]");   \
    asm("stp X28, X19, [SP,#0x110]");   \
    asm("stp X20, X21, [SP,#0x120]");   \
    asm("stp X22, X23, [SP,#0x130]");   \
    asm("stp X24, X25, [SP,#0x140]");   \
    asm("stp X26, X27, [SP,#0x150]");   \
    asm("sub SP, SP, #0x200");          \
    asm("stp D0, D1, [SP,#0x00]");      \
    asm("stp D2, D3, [SP,#0x10]");      \
    asm("stp D4, D5, [SP,#0x20]");      \
    asm("stp D6, D7, [SP,#0x30]");      \
    asm("stp D8, D9, [SP,#0x40]");      \
    asm("stp D10, D11, [SP,#0x50]");    \
    asm("stp D12, D13, [SP,#0x60]");    \
    asm("stp D14, D15, [SP,#0x70]");    \
    asm("stp D16, D17, [SP,#0x80]");    \
    asm("stp D18, D19, [SP,#0x90]");    \
    asm("stp D20, D21, [SP,#0x100]");   \
    asm("stp D22, D23, [SP,#0x110]");   \
    asm("stp D24, D25, [SP,#0x120]");   \
    asm("stp D26, D27, [SP,#0x130]");   \
    asm("stp D28, D29, [SP,#0x140]");   \
    asm("stp D30, D31, [SP,#0x150]");

#define POP_ALL()                       \
    asm("ldp D30, D31, [SP,#0x150]");   \
    asm("ldp D28, D29, [SP,#0x140]");   \
    asm("ldp D26, D27, [SP,#0x130]");   \
    asm("ldp D24, D25, [SP,#0x120]");   \
    asm("ldp D22, D23, [SP,#0x110]");   \
    asm("ldp D20, D21, [SP,#0x100]");   \
    asm("ldp D18, D19, [SP,#0x90]");    \
    asm("ldp D16, D17, [SP,#0x80]");    \
    asm("ldp D14, D15, [SP,#0x70]");    \
    asm("ldp D12, D13, [SP,#0x60]");    \
    asm("ldp D10, D11, [SP,#0x50]");    \
    asm("ldp D8, D9, [SP,#0x40]");      \
    asm("ldp D6, D7, [SP,#0x30]");      \
    asm("ldp D4, D5, [SP,#0x20]");      \
    asm("ldp D2, D3, [SP,#0x10]");      \
    asm("ldp D0, D1, [SP,#0x0]");       \
    asm("add SP, SP, #0x200");          \
    asm("ldp X26, X27, [SP,#0x150]");   \
    asm("ldp X24, X25, [SP,#0x140]");   \
    asm("ldp X22, X23, [SP,#0x130]");   \
    asm("ldp X20, X21, [SP,#0x120]");   \
    asm("ldp X28, X19, [SP,#0x110]");   \
    asm("ldp X16, X17, [SP,#0x100]");   \
    asm("ldp X14, X15, [SP,#0x90]");    \
    asm("ldp X12, X13, [SP,#0x80]");    \
    asm("ldp X10, X11, [SP,#0x70]");    \
    asm("ldp X8, X9, [SP,#0x60]");      \
    asm("ldp X6, X7, [SP,#0x50]");      \
    asm("ldp X4, X5, [SP,#0x40]");      \
    asm("ldp X2, X3, [SP,#0x30]");      \
    asm("ldp X0, X1, [SP,#0x20]");      \
    asm("ldp X29, X30, [SP,#0x10]");    \
    asm("msr NZCV, x18");               \
    asm("add SP, SP, #0x200");


void __attribute__((naked)) hook_template(){

    PUSH_ALL();

    // blr用于实现函数调用和返回机制，保存返回地址并跳转到指定地址，用于函数间的跳转和调用。
    // br用于无条件跳转到指定地址，通常用于简单的无条件分支需求，不涉及函数调用和返回。
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n) = 0x58000070;                           // ldr x16, #0xc
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n+4) = 0xd63f0200;                         // blr x16
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n+8) = 0x14000003;                         // b #0xc
    asm("mov x0,x0");      // *(long *) ((char *) (a) + n+12) = reinterpret_cast<long>(address2); // 填充hook函数地址
    asm("mov x0,x0");

    POP_ALL();

    asm("mov x0,x0");      // *(int *) ((char *) (a) + m + 24) = 0x58000050;;          // ldr x16,8
    asm("mov x0,x0");      // *(int *) ((char *) (a) + m + 28) = 0xd61f0200;           // br x16
    asm("mov x0,x0");      // *(long *) ((char *) (a) + m + 32) = ret_addr;            // 填充hook函数地址
    asm("mov x0,x0");

    // 指令膨胀，为修复特殊指令预留空间
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");
    asm("mov x0,x0");   asm("mov x0,x0");

}

void __attribute__((naked)) hook_template2(){
    asm("mov x6,x6");
    asm("sub SP, SP, #0x1000");
    asm("ldr X30, [SP]");
    asm("add SP, SP, #0x1000");

    PUSH_ALL();

    // blr用于实现函数调用和返回机制，保存返回地址并跳转到指定地址，用于函数间的跳转和调用。
    // br用于无条件跳转到指定地址，通常用于简单的无条件分支需求，不涉及函数调用和返回。
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n) = 0x58000070;                           // ldr x16, #0xc
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n+4) = 0xd63f0200;                         // blr x16
    asm("mov x0,x0");      // *(int *) ((char *) (a) + n+8) = 0x14000003;                         // b #0xc
    asm("mov x0,x0");      // *(long *) ((char *) (a) + n+12) = reinterpret_cast<long>(address2); // 填充hook函数地址
    asm("mov x0,x0");

    POP_ALL();

    asm("ret");
}

int old_code1 = 0;
int old_code2 = 0;
int old_code3 = 0;
int old_code4 = 0;


void register_hook(void* address1, void* address2){

    // 设置内存读写执行权限
    mprotect((void *) PAGE_START((long) address1), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
    mprotect((void *) PAGE_START((long) address2), PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 保存原函数指令
    old_code1 = *(int *) ((char *) address1);
    old_code2 = *(int *) ((char *) address1 + 4);
    old_code3 = *(int *) ((char *) address1 + 8);
    old_code4 = *(int *) ((char *) address1 + 12);
    long ret_addr = (long)address1 + 0x10;

    // 申请跳板空间，将模板拷贝到申请的空间中
    int template_size = 0x300;
    void* a = malloc(template_size);
    if (a == nullptr){
        return;
    }

    memcpy(a, (void*)hook_template, template_size);
    int page_num = (PAGE_START((long) a + template_size) - PAGE_START((long) a))/PAGE_SIZE + 1;
    mprotect((void *) PAGE_START((long) a), PAGE_SIZE * page_num, PROT_READ | PROT_WRITE | PROT_EXEC);

    // 将原函数头部改为跳板指令
    *(int *) ((char *) address1) = 0x58000050;              // ldr x16,8
    *(int *) ((char *) address1 + 4) = 0xd61f0200;          // br x16
    *(long *) ((char *) address1 + 8) = (long)a;            // 跳板空间地址

    // 将模板预留的位置 1 填充为跳转 address2 的跳板指令
    int n=0x44;
    n=0x88;
    *(int *) ((char *) (a) + n) = 0x58000070;               // ldr x16, #0xC
    *(int *) ((char *) (a) + n+4) = 0xd63f0200;             // blr x16
    *(int *) ((char *) (a) + n+8) = 0x14000003;             // b #0xC
    *(long *) ((char *) (a) + n+12) = (long)address2;       // 填充hook函数地址

    int m=0xA4;
    m = 0x124;
    *(int *) ((char *) (a) + m) = old_code1;
    *(int *) ((char *) (a) + m + 4) = old_code2;
    *(int *) ((char *) (a) + m + 8) = old_code3;
    *(int *) ((char *) (a) + m + 12) = old_code4;

    *(int *) ((char *) (a) + m + 24) = 0x58000050;;          // ldr x16,8
    *(int *) ((char *) (a) + m + 28) = 0xd61f0200;           // br x16
    *(long *) ((char *) (a) + m + 32) = ret_addr;            // 填充hook函数地址

    /* 这里改回原本的内存属性虽然在 maps 中显示为 r-xp，但却由一个 r-xp 区域变成了多个连续的 r-xp 区域
     *
     * 7e49e40000-7e49e86000 r--p 00000000 fe:09 18184502                       /system/lib64/libinput.so
     * 7e49e86000-7e49eb9000 r-xp 00046000 fe:09 18184502                       /system/lib64/libinput.so
     * 7e49eb9000-7e49ebb000 r-xp 00079000 fe:09 18184502                       /system/lib64/libinput.so
     * 7e49ebb000-7e49f31000 r-xp 0007b000 fe:09 18184502                       /system/lib64/libinput.so
     * 7e49f31000-7e49f3b000 r--p 000f1000 fe:09 18184502                       /system/lib64/libinput.so
     * 7e49f3b000-7e49f3c000 rw-p 000fa000 fe:09 18184502                       /system/lib64/libinput.so
     *
     * 这是因为 Linux 的 VMA（虚拟内存区域）机制
     * 内核以 vm_area_struct 结构表示一段连续、具有相同属性的虚拟内存区域；
     * 每次调用 mprotect() 修改某段页的属性，内核会把这段原来的 vm_area_struct 拆分成：
     *   1. 保留前段不变；
     *   2. 中间段属性变化 → 拆成新 VMA；
     *   3. 后段不变；
     * 即使后面用 mprotect() 改回去，也不会自动合并这些 vm_area_struct
     *
     *
     *
     * 后面可以尝试的解决方法：
     * 1. 修改权限时，不只修改一页，而是将 so 的代码段全部修改为 r-xp，改完代码后再统一改回 r-xp；
     * */

    mprotect((void *) PAGE_START((long) address1), PAGE_SIZE, PROT_READ | PROT_EXEC);
    sleep(0);
//    __asm__ __volatile__ ("dmb ishst" ::: "memory");         // 数据同步屏障指令，它会确保所有存储访问都已经完成。
}

void register_hook_with_leave(void* address1, void* address2, void* address3){

    // 设置内存读写执行权限
    mprotect((void *) PAGE_START((long) address1), PAGE_SIZE, PROT_WRITE | PROT_READ | PROT_EXEC);
    mprotect((void *) PAGE_START((long) address2), PAGE_SIZE, PROT_WRITE | PROT_READ | PROT_EXEC);

    // 保存原函数指令
    old_code1 = *(int *) ((char *) address1);
    old_code2 = *(int *) ((char *) address1 + 4);
    old_code3 = *(int *) ((char *) address1 + 8);
    old_code4 = *(int *) ((char *) address1 + 12);
    long ret_addr = (long)address1 + 0x10;

    // 申请跳板空间，将模板拷贝到申请的空间中
    int template_size = 0x300;
    void* a = malloc(template_size);
    if (a == nullptr){
        return;
    }

    memcpy(a, (void*)hook_template, template_size);
    int page_num = (PAGE_START((long) a + template_size) - PAGE_START((long) a))/PAGE_SIZE + 1;
    mprotect((void *) PAGE_START((long) a), PAGE_SIZE * page_num, PROT_WRITE | PROT_READ | PROT_EXEC);



    // ============================================================= //

    // 申请跳板空间，将模板拷贝到申请的空间中
    int template_size2 = 0x300;
    void* a2 = malloc(template_size2);
    if (a2 == nullptr){
        return;
    }

    memcpy(a2, (void*)hook_template2, template_size2);
    int page_num2 = (PAGE_START((long) a2 + template_size2) - PAGE_START((long) a2))/PAGE_SIZE + 1;
    mprotect((void *) PAGE_START((long) a2), PAGE_SIZE * page_num2, PROT_WRITE | PROT_READ | PROT_EXEC);

    // 将模板预留的位置 1 填充为跳转 address2 的跳板指令
    int n2=0x88+4*4;
    *(int *) ((char *) (a2) + n2) = 0x58000070;               // ldr x16, #0xC
    *(int *) ((char *) (a2) + n2+4) = 0xd63f0200;             // blr x16
    *(int *) ((char *) (a2) + n2+8) = 0x14000003;             // b #0xC
    *(long *) ((char *) (a2) + n2+12) = (long)address3;       // 填充hook函数地址

    // ============================================================= //

    // 将原函数头部改为跳板指令
    *(int *) ((char *) address1) = 0x58000050;              // ldr x16,8
    *(int *) ((char *) address1 + 4) = 0xd61f0200;          // br x16
    *(long *) ((char *) address1 + 8) = (long)a;            // 跳板空间地址

    // 将模板预留的位置 1 填充为跳转 address2 的跳板指令
    int n=0x88;
    *(int *) ((char *) (a) + n) = 0x58000070;               // ldr x16, #0xC
    *(int *) ((char *) (a) + n+4) = 0xd63f0200;             // blr x16
    *(int *) ((char *) (a) + n+8) = 0x14000003;             // b #0xC
    *(long *) ((char *) (a) + n+12) = (long)address2;       // 填充hook函数地址


    int nnn=0x124;
    *(int *) ((char *) (a) + nnn) = 0xD14007FF;               // sub sp, sp, 0x1000
    *(int *) ((char *) (a) + nnn+4) = 0xF90003FE;               // str x30, [sp]
    *(int *) ((char *) (a) + nnn+8) = 0x914007FF;               // add sp, sp, 0x1000

    int nn=0x134;
    *(int *) ((char *) (a) + nn) = 0x5800005E;               // ldr x30, #0x8
    *(int *) ((char *) (a) + nn+4) = 0x14000003;             // b #0xC
    *(long *) ((char *) (a) + nn+8) = (long)a2;              // 填充hook函数地址

    int m=0x144;
    *(int *) ((char *) (a) + m) = old_code1;
    *(int *) ((char *) (a) + m + 4) = old_code2;
    *(int *) ((char *) (a) + m + 8) = old_code3;
    *(int *) ((char *) (a) + m + 12) = old_code4;

    *(int *) ((char *) (a) + m + 24) = 0x58000050;;          // ldr x16,8
    *(int *) ((char *) (a) + m + 28) = 0xd61f0200;           // br x16
    *(long *) ((char *) (a) + m + 32) = ret_addr;            // 填充hook函数地址


    sleep(0);

//    __asm__ __volatile__ ("dmb ishst" ::: "memory");         // 数据同步屏障指令，它会确保所有存储访问都已经完成。
}


void unregister_hook_pass_(void* address1){

    mprotect((void *) PAGE_START((long) address1), PAGE_SIZE, PROT_WRITE | PROT_READ | PROT_EXEC);

    *(int *) ((char *) address1) = old_code1;
    *(int *) ((char *) address1 + 4) = old_code2;
    *(int *) ((char *) address1 + 8) = old_code3;
    *(int *) ((char *) address1 + 12) = old_code4;

    sleep(0);
}


