#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 101

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);


#define MAXDIGITS 100 /* maximum length bignum */
#define PLUS 1        /* positive sign bit */
#define MINUS -1      /* negative sign bit */
#define MAX(a, b) ((a > b) ? a : b)

#ifndef strlcpy
#define strlcpy(dst, src, sz) snprintf((dst), (sz), "%s", (src))
#endif

typedef struct {
    char digits[MAXDIGITS]; /* represent the number */
    int signbit;            /* 1 if positive, -1 if negative */
    int lastdigit;          /* index of high-order digit */
} bignum;

int add_bignum(bignum *a, bignum *b, bignum *c);
int compare_bignum(bignum *a, bignum *b);
int subtract_bignum(bignum *a, bignum *b, bignum *c);


void int_to_bignum(int s, bignum *n)
{
    if (s >= 0)
        n->signbit = PLUS;
    else
        n->signbit = MINUS;
    int t = abs(s);
    snprintf(n->digits, sizeof(n->digits), "%d", t);
    n->lastdigit = strlen(n->digits);
}

void initialize_bignum(bignum *n)
{
    int_to_bignum(0, n);
}

int add_bignum(bignum *a, bignum *b, bignum *c)

{
    int carry; /* carry digit */

    int i, j;  //, op = 0;
               /* counter */

    int n_carry;  //, temp;
    initialize_bignum(c);
    if (a->signbit == b->signbit)
        c->signbit = a->signbit;
    else {
        if (a->signbit == MINUS) {
            a->signbit = PLUS;
            n_carry = subtract_bignum(b, a, c);
            a->signbit = MINUS;
        } else {
            b->signbit = PLUS;
            n_carry = subtract_bignum(a, b, c);
            b->signbit = MINUS;
        }
        return n_carry;
    }

    if (a->lastdigit < b->lastdigit)

        return add_bignum(b, a, c);

    int k = c->lastdigit = a->lastdigit + 1;

    c->digits[k--] = '\0';
    carry = 0;
    n_carry = 0;
    for (i = b->lastdigit - 1, j = a->lastdigit - 1; i >= 0; i--, j--) {
        carry = b->digits[i] - '0' + a->digits[j] - '0' + carry;
        c->digits[k--] = (carry % 10) + '0';
        carry = carry / 10;
        if (carry)
            n_carry++;
    }
    for (; j >= 0; j--) {
        carry = a->digits[j] - '0' + carry;
        c->digits[k--] = (carry % 10) + '0';
        carry = carry / 10;
        if (carry)
            n_carry++;
    }
    if (carry)
        c->digits[k] = carry + '0';
    else {
        char string[MAXDIGITS];
        strlcpy(string, &c->digits[1], MAXDIGITS);
        strlcpy(c->digits, string, MAXDIGITS);
        c->lastdigit = c->lastdigit - k - 1;
    }
    return n_carry;
}

int subtract_bignum(bignum *a, bignum *b, bignum *c)
{
    // int borrow; /* has anything been borrowed? */
    // int v; /* placeholder digit */
    register int i, j, op = 0; /* counter */
    int n_borrow;
    int temp;
    c->signbit = PLUS;

    if ((a->signbit == MINUS) || (b->signbit == MINUS))

    {
        b->signbit = -1 * b->signbit;

        n_borrow = add_bignum(a, b, c);

        b->signbit = -1 * b->signbit;

        return n_borrow;
    }
    if (compare_bignum(a, b) == PLUS) {
        n_borrow = subtract_bignum(b, a, c);
        c->signbit = MINUS;
        return n_borrow;
    }
    int k = c->lastdigit = MAX(a->lastdigit, b->lastdigit);
    n_borrow = 0;
    c->digits[k--] = '\0';
    for (i = a->lastdigit - 1, j = b->lastdigit - 1; j >= 0; i--, j--) {
        temp = a->digits[i] - '0' - (b->digits[j] - '0' + op);

        if (temp < 0) {
            temp += 10;
            op = 1;
            n_borrow++;
        } else
            op = 0;
        c->digits[k--] = temp + '0';
    }
    while (op) {
        temp = a->digits[i--] - op - '0';
        if (temp < 0) {
            temp += 10;
            op = 1;
            n_borrow++;
        } else
            op = 0;
        c->digits[k--] = temp + '0';
    }
    for (; i >= 0; i--)
        c->digits[k--] = a->digits[i];
    for (i = 0; !(c->digits[i] - '0'); i++)
        ;
    c->lastdigit = c->lastdigit - i;
    if (i == a->lastdigit)
        strlcpy(c->digits, "0", MAXDIGITS);
    else {
        char string[MAXDIGITS];
        strlcpy(string, &c->digits[i], MAXDIGITS);
        strlcpy(c->digits, string, MAXDIGITS);
    }
    return n_borrow;
}

int compare_bignum(bignum *a, bignum *b)

{
    int i; /* counter */
    if ((a->signbit == MINUS) && (b->signbit == PLUS))
        return (PLUS);

    if ((a->signbit == PLUS) && (b->signbit == MINUS))
        return (MINUS);

    if (b->lastdigit > a->lastdigit)
        return (PLUS * a->signbit);

    if (a->lastdigit > b->lastdigit)
        return (MINUS * a->signbit);

    for (i = 0; i < a->lastdigit; i++) {
        if (a->digits[i] > b->digits[i])
            return (MINUS * a->signbit);
        if (b->digits[i] > a->digits[i])
            return (PLUS * a->signbit);
    }
    return (0);
}

void multiply_bignum(bignum *a, bignum *b, bignum *c)
{
    // long int n_d;
    register long int i, j, k = 0;
    short int num1[MAXDIGITS], num2[MAXDIGITS], of = 0, res[MAXDIGITS] = {0};
    // n_d = (a->lastdigit < b->lastdigit) ? b->lastdigit : a->lastdigit;
    // n_d++;
    for (i = 0, j = a->lastdigit - 1; i < a->lastdigit; i++, j--)
        num1[i] = a->digits[j] - 48;
    for (i = 0, j = b->lastdigit - 1; i < b->lastdigit; j--, i++)
        num2[i] = b->digits[j] - 48;
    res[0] = 0;
    for (j = 0; j < b->lastdigit; j++) {
        for (i = 0, k = j; i < a->lastdigit || of; k++, i++) {
            if (i < a->lastdigit)

                res[k] += num1[i] * num2[j] + of;

            else
                res[k] += of;

            of = res[k] / 10;

            res[k] = res[k] % 10;
        }
    }
    for (i = k - 1, j = 0; i >= 0; i--, j++)
        c->digits[j] = res[i] + 48;
    c->digits[j] = '\0';
    c->lastdigit = k;
    c->signbit = a->signbit * b->signbit;
}

void copy(bignum *a, bignum *b)
{
    a->lastdigit = b->lastdigit;
    a->signbit = b->signbit;
    strlcpy(a->digits, b->digits, MAXDIGITS);
}

static bignum fib_sequence(unsigned long long k)
{
    bignum a, b;  //, c;
    bignum big_two;
    int_to_bignum(0, &a);
    int_to_bignum(1, &b);
    int_to_bignum(2, &big_two);
    for (int i = 31 - __builtin_clz(k); i >= 0; i--) {
        bignum t1, t2;
        bignum tmp1, tmp2;
        multiply_bignum(&b, &big_two, &tmp1);
        (void) subtract_bignum(&tmp1, &a, &tmp2);
        multiply_bignum(&a, &tmp2, &t1);

        multiply_bignum(&a, &a, &tmp1);
        multiply_bignum(&b, &b, &tmp2);
        (void) add_bignum(&tmp1, &tmp2, &t2);
        copy(&a, &t1);
        copy(&b, &t2);
        if ((k & (1 << i)) > 0) {
            (void) add_bignum(&a, &b, &t1);
            copy(&a, &b);
            copy(&b, &t1);
        }
    }
    // printk("%s\n",a.digits);
    return a;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    char kbuf[MAXDIGITS] = {0};
    bignum res = fib_sequence(*offset);
    snprintf(kbuf, MAXDIGITS, "%s", res.digits);
    copy_to_user(buf, kbuf, MAXDIGITS);
    return 0;
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;  //(u128_t){.upper = 0, .lower = 1};
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
