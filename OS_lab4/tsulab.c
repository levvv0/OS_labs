#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/random.h>
#include <linux/workqueue.h>

#define WORK_COUNT 5U
#define MAX_ACTIVE 5

struct lab_work {
    struct work_struct work;
    unsigned int id;
    unsigned int stages;
    unsigned int delay_ms;
    unsigned int completed;
};

static struct workqueue_struct *queue;
static struct lab_work items[WORK_COUNT];

static void lab_work_handler(struct work_struct *work)
{
    struct lab_work *item = container_of(work, struct lab_work, work);
    unsigned int stage;

    pr_info("tsulab: work %u started\n", item->id);

    for (stage = 1; stage <= item->stages; ++stage) {
        pr_info("tsulab: work %u, stage %u/%u, sleep %u ms\n",
                item->id, stage, item->stages, item->delay_ms);
        msleep(item->delay_ms);
        item->completed = stage;
    }

    pr_info("tsulab: work %u finished\n", item->id);
}

static int __init tsulab_init(void)
{
    unsigned int i;

    queue = alloc_workqueue("tsulab_queue", WQ_UNBOUND, MAX_ACTIVE);
    if (!queue)
        return -ENOMEM;

    pr_info("Welcome to the Tomsk State University\n");

    for (i = 0; i < WORK_COUNT; ++i) {
        items[i].id = i + 1;
        items[i].stages = 2 + get_random_u32_below(4);
        items[i].delay_ms = 1000 + get_random_u32_below(1001);
        INIT_WORK(&items[i].work, lab_work_handler);

        pr_info("tsulab: queueing work %u, stages=%u, delay=%u ms\n",
                items[i].id, items[i].stages, items[i].delay_ms);
        queue_work(queue, &items[i].work);
    }

    pr_info("tsulab: %u works queued, init finished\n", WORK_COUNT);
    return 0;
}

static void __exit tsulab_exit(void)
{
    unsigned int i;

    pr_info("tsulab: unloading, waiting for all works\n");
    flush_workqueue(queue);

    for (i = 0; i < WORK_COUNT; ++i)
        pr_info("tsulab: result work %u: completed %u/%u stages\n",
                items[i].id, items[i].completed, items[i].stages);

    destroy_workqueue(queue);
    pr_info("tsulab: all %u works completed, queue destroyed\n", WORK_COUNT);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TSU lab: long operations in a private workqueue");
