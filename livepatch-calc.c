#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/livepatch.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/version.h>

#include "expression.h"

#define GET_NUM_LP(n) ((n) >> 4)

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Patch calc kernel module");
MODULE_VERSION("0.1");

int GET_FRAC_LP(int n)
{
    int x = n & 15;
    if (x & 8)
        return -(((~x) & 15) + 1);

    return x;
}

int FP2INT_LP(int n, int d)
{
    /* Tailed zero counting */
    while (n && n % 10 == 0) {
        ++d;
        n /= 10;
    }
    if (d == -1) {
        n *= 10;
        --d;
    }

    return ((n << 4) | (d & 15));
}

/* Fibonacci Number calculating via fast doubling */
int fibn(int n)
{
    int t0 = 1, t1 = 1;  // For F[n], F[n+1]
    int t3 = 1, t4 = 0;  // For F[2n], F[2n+1]

    int i = 1;

    if (n <= 0)
        return 0;
    else if (n > 40)  // Limitation for fixed point number
        return -1;

    while (i < n) {
        if ((i << 1) <= n) {
            t4 = t1 * t1 + t0 * t0;
            t3 = t0 * (2 * t1 - t0);
            t0 = t3;
            t1 = t4;
            i = i << 1;
        } else {
            t0 = t3;
            t3 = t4;
            t4 = t0 + t4;
            i++;
        }
    }
    return t3;
}

void livepatch_nop_cleanup(struct expr_func *f, void *c)
{
    /* suppress compilation warnings */
    (void) f;
    (void) c;
}

int livepatch_nop(struct expr_func *f, vec_expr_t args, void *c)
{
    (void) args;
    (void) c;
    pr_info("function nop is now patched\n");
    return 640;
}

void livepatch_fib_cleanup(struct expr_func *f, void *c)
{
    (void) f;
    (void) c;
}

int livepatch_fib(struct expr_func *f, vec_expr_t args, void *c)
{
    (void) args;
    (void) c;

    pr_info("function fib is now patched, type: %d\n", args.buf[0].type);

    if (args.buf[0].type != 25) {  // For marco OP_CONST
        pr_err("Input argument for fib() error...");
        return -1;
    }

    struct expr x = vec_pop(&args);
    int ipn = GET_NUM_LP(x.param.num.value),
        cnt = GET_FRAC_LP(x.param.num.value);

    while (cnt--)
        ipn *= 10;

    pr_info("%d, %d, %d, %d\n", x.param.num.value, ipn, cnt,
            FP2INT_LP(ipn, cnt));

    return FP2INT_LP(fibn(ipn), 0);
}

/* clang-format off */
static struct klp_func funcs[] = {
    {
        .old_name = "user_func_nop",
        .new_func = livepatch_nop,
    },
    {
        .old_name = "user_func_nop_cleanup",
        .new_func = livepatch_nop_cleanup,
    },
    {
        .old_name = "user_func_fib",
        .new_func = livepatch_fib,
    },
    {
        .old_name = "user_func_fib_cleanup",
        .new_func = livepatch_fib_cleanup,
    },
    {},
};

static struct klp_object objs[] = {
    {
        .name = "calc",
        .funcs = funcs,
    },
    {},
};
/* clang-format on */

static struct klp_patch patch = {
    .mod = THIS_MODULE,
    .objs = objs,
};

static int livepatch_calc_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
    return klp_enable_patch(&patch);
#else
    int ret = klp_register_patch(&patch);
    if (ret)
        return ret;
    ret = klp_enable_patch(&patch);
    if (ret) {
        WARN_ON(klp_unregister_patch(&patch));
        return ret;
    }
    return 0;
#endif
}

static void livepatch_calc_exit(void)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
    WARN_ON(klp_unregister_patch(&patch));
#endif
}

module_init(livepatch_calc_init);
module_exit(livepatch_calc_exit);
MODULE_INFO(livepatch, "Y");
