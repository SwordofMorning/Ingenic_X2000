#include "string.h"
#include <driver/hrtimer.h>
#include <cpu/irqflags.h>
struct hrtimer_cpu_base                 hrtimer_base;

static void hrtimer_list_add(struct hrtimer_cpu_base *cpu_base,
                struct hrtimer *timer)
{
    struct hrtimer *tmp;

    list_for_each_entry(tmp, &cpu_base->queue, node) {
        if (tmp->expires_cycles > timer->expires_cycles) {
            list_add_tail(&timer->node, &tmp->node);
            return;
        }
    }
    list_add_tail(&timer->node, &cpu_base->queue);
}

static inline void hrtimer_list_del(struct hrtimer *timer)
{
    list_del(&timer->node);
}

static inline struct hrtimer *hrtimer_getnext(struct hrtimer_cpu_base *cpu_base)
{
    if (list_empty(&cpu_base->queue)) {
        return NULL;
    } else {
        return list_first_entry(&cpu_base->queue, struct hrtimer, node);
    }
}

static void hrtimer_set_next_count(uint64_t count)
{
    uint64_t now = systick_get_count();
    uint64_t relative_count = count - now;

    if (now >= count)
        relative_count = 1;

    systick_set_event_count(relative_count);
}

/*
 * Reprogram the event source with checking both queues for the
 * next event
 * Called with interrupts disabled and base->lock held
 */
static void hrtimer_force_reprogram(struct hrtimer_cpu_base *cpu_base, int skip_equal)
{
    uint64_t expires_cycles_next;

    expires_cycles_next = TIME_MAX;

    if (skip_equal && expires_cycles_next == cpu_base->expires_cycles_next)
        return;

    cpu_base->expires_cycles_next = expires_cycles_next;

    if (cpu_base->expires_cycles_next != TIME_MAX) {
        hrtimer_set_next_count(cpu_base->expires_cycles_next);
    }
}

/*
 * Shared reprogramming for clock_realtime and clock_monotonic
 *
 * When a timer is enqueued and expires earlier than the already enqueued
 * timers, we have to check, whether it expires earlier than the timer for
 * which the clock event device was armed.
 *
 * Called with interrupts disabled and base->cpu_base.lock held
 */
static int hrtimer_reprogram(struct hrtimer *timer)
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;
    uint64_t expires_cycles = timer->expires_cycles;

    /*
     * When the callback is running, we do not reprogram the clock event
     * device. The timer callback is executed in the hrtimer_interrupt
     * context. The reprogramming is handled either by the softirq, which
     * called the callback or at the end of the hrtimer_interrupt.
     */
    if (hrtimer_callback_running(timer)) {
        return 0;
    }

    if (expires_cycles >= cpu_base->expires_cycles_next) {
        return 0;
    }

    /*
     * Clockevents returns -ETIME, when the event was in the past.
     */
    hrtimer_set_next_count(expires_cycles);
    cpu_base->expires_cycles_next = expires_cycles;

    return 0;
}

/*
 * When High resolution timers are active, try to reprogram. Note, that in case
 * the state has HRTIMER_STATE_CALLBACK set, no reprogramming and no expiry
 * check happens. The timer gets enqueued into the rbtree. The reprogramming
 * and expiry check is done in the hrtimer_interrupt or in the softirq.
 */
static inline int hrtimer_enqueue_reprogram(struct hrtimer *timer)
{
    return hrtimer_reprogram(timer);
}

/*
 * enqueue_hrtimer - internal function to (re)start a timer
 *
 * The timer is inserted in expiry order. Insertion into the
 * red black tree is O(log(n)). Must hold the base lock.
 *
 * Returns 1 when the new timer is the leftmost timer in the tree.
 */
static int enqueue_hrtimer(struct hrtimer *timer)
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;

    hrtimer_list_add(cpu_base, timer);
    cpu_base->active_bases = TRUE;

    /*
     * HRTIMER_STATE_ENQUEUED is or'ed to the current state to preserve the
     * state of a possibly running callback.
     */
    timer->state |= HRTIMER_STATE_ENQUEUED;

    return (&timer->node == cpu_base->queue.next);
}

/*
 * __remove_hrtimer - internal function to remove a timer
 *
 * Caller must hold the base lock.
 *
 * High resolution timer mode reprograms the clock event device when the
 * timer is the one which expires next. The caller can disable this by setting
 * reprogram to zero. This is useful, when the context does a reprogramming
 * anyway (e.g. timer interrupt)
 */
static void __remove_hrtimer(struct hrtimer *timer,
                 unsigned long newstate, int reprogram)
{
    struct hrtimer *next_timer;
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;
    if (!(timer->state & HRTIMER_STATE_ENQUEUED)) {
        goto out;
    }


    next_timer = hrtimer_getnext(cpu_base);
    hrtimer_list_del(timer);
    if (timer == next_timer) {
        /* Reprogram the clock event device. if enabled */
        if (reprogram && cpu_base->expires_cycles_next == timer->expires_cycles) {
                hrtimer_force_reprogram(cpu_base, 1);
        }
    }

    if (!hrtimer_getnext(cpu_base)) {
        cpu_base->active_bases = FALSE;
    }
out:
    timer->state = newstate;
}

/*
 * remove hrtimer, called with base lock held
 */
static inline int remove_hrtimer(struct hrtimer *timer)
{
    if (hrtimer_is_queued(timer)) {
        unsigned long state;
        /*
         * We must preserve the CALLBACK state flag here,
         * otherwise we could move the timer base in
         * switch_hrtimer_base.
         */
        state = timer->state & HRTIMER_STATE_CALLBACK;
        __remove_hrtimer(timer, state, 1);
        return 1;
    }
    return 0;
}

int hrtimer_start_at_expires(struct hrtimer *timer, uint64_t expires_cycles)
{
    int ret, leftmost;
    unsigned int flag;

    local_irq_save(flag);

    /* Remove an active timer from the queue: */
    ret = remove_hrtimer(timer);

    timer->expires_cycles = expires_cycles;

    leftmost = enqueue_hrtimer(timer);
    if (leftmost){
        hrtimer_enqueue_reprogram(timer);
    }

    local_irq_restore(flag);

    return ret;
}

int hrtimer_start(struct hrtimer *timer, uint64_t count)
{
    int ret, leftmost;
    unsigned int flag;

    local_irq_save(flag);

    /* Remove an active timer from the queue: */
    ret = remove_hrtimer(timer);

    timer->expires_cycles = count + systick_get_count();

    leftmost = enqueue_hrtimer(timer);
    if (leftmost){
        hrtimer_enqueue_reprogram(timer);
    }

    local_irq_restore(flag);
    return ret;
}

/*
 * similar as hrtimer_start, but it more sample then hrtimer_start.
 * it is invoked ONLY when there is hrtimer handler callback context
 */
void hrtimer_restart(struct hrtimer *timer, uint64_t count)
{
    timer->expires_cycles = count + systick_get_count();

    enqueue_hrtimer(timer);
}

void hrtimer_restart_at_expires(struct hrtimer *timer, uint64_t expires_cycles)
{
    timer->expires_cycles = expires_cycles;

    enqueue_hrtimer(timer);
}

int hrtimer_try_to_cancel(struct hrtimer *timer)
{
    int ret = -1;
    unsigned int flag;

    local_irq_save(flag);

    if (!hrtimer_callback_running(timer))
        ret = remove_hrtimer(timer);

    local_irq_restore(flag);

    return ret;
}

int hrtimer_cancel(struct hrtimer *timer)
{
    for (;;) {
        int ret = hrtimer_try_to_cancel(timer);

        if (ret >= 0)
            return ret;
    }

    return 0;
}

void hrtimer_init(struct hrtimer *timer, void (*function)(struct hrtimer *))
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;

    memset(timer, 0, sizeof(struct hrtimer));

    timer->cpu_base = cpu_base;
    timer->function = function;
    INIT_LIST_HEAD(&timer->node);
}

static void __run_hrtimer(struct hrtimer *timer)
{
    __remove_hrtimer(timer, HRTIMER_STATE_CALLBACK, 0);
    timer->function(timer);
    timer->state &= ~HRTIMER_STATE_CALLBACK;
}


/*
 * High resolution timer interrupt
 * Called with interrupts disabled
 */
static void hrtimer_interrupt(void)
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;
    struct hrtimer *timer;

    uint64_t expires_cycles_next;


    expires_cycles_next = TIME_MAX;
    cpu_base->expires_cycles_next = TIME_MAX;

    if (!cpu_base->active_bases) {
        return ;
    }

    while ((timer = hrtimer_getnext(cpu_base))) {
        if (systick_get_count() < timer->expires_cycles) {
            // printf("%s: now: %lld, expires: %lld\n", __func__, now, timer->expires);
            if (timer->expires_cycles < expires_cycles_next) {
                expires_cycles_next = timer->expires_cycles;
            }
            break;
        }

        __run_hrtimer(timer);
    }

    /*
     * Store the new expiry value so the migration code can verify
     * against it.
     */
    cpu_base->expires_cycles_next = expires_cycles_next;

    /* Reprogramming necessary ? */
    if (expires_cycles_next != TIME_MAX) {
        hrtimer_set_next_count(expires_cycles_next);
    }

}

void hrtimer_reset_irq_time(void)
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;

    if (cpu_base->expires_cycles_next != TIME_MAX)
        hrtimer_set_next_count(cpu_base->expires_cycles_next);
}


void hrtimer_core_init(void)
{
    struct hrtimer_cpu_base *cpu_base = &hrtimer_base;

    cpu_base->expires_cycles_next = TIME_MAX;

    INIT_LIST_HEAD(&cpu_base->queue);
    systick_set_event_callback(hrtimer_interrupt);
}