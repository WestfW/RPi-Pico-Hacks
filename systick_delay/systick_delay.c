#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/gpio.h"
#include "pico/bootrom.h"

/*
 * Control test behavior
 */

static int disable_interrupts = 1;  // disable interrupts around delay function

static int flush_cache = 0;         // flush instruction cache before delay function
                                    // (useful for measuring cache refill delays.)

static int print_individual = 1;    // Print individual test results
                                    //  (in addition to summary)



/*
   Delay for N cycles, using sysTick Timer.

   Systick must be running.  count should be significantly less than
   the LOAD value, so this is probably good for
       100 cycles < delay < 800us
   (or less) with a 1ms systick interrupt.

   (likewise, obviously not accurate to 1 cycle.
     Probably accurate to within ~20 cycles, except for interrupts.)
     (remember SYSTICK counts DOWN!)
     eeeeeE--------SeeeeeeeeeeeeeeeeeeR  (no wrap)
     --SeeeeeeeeeeeeeeeeeeeeeeeeeE----R  (wrap - end time E > start time S)
*/

#define GET_SYSTICK systick_hw->cvr
#define GET_SYSTICKMAX systick_hw->rvr
 


void sysTickDelay (uint32_t count) __attribute__((noinline));
void sysTickDelay (uint32_t count)
{
  uint32_t reload = GET_SYSTICKMAX;
  int32_t start, end, now;

  start = GET_SYSTICK;
  end = start - count;
  if (end > 0) {
    while (1) {
      now = GET_SYSTICK;
      if ((now <= end) || (now > start))
        return;
    }
  } else {
    end += reload;  // wrap the end time
    while (1) {
        now = GET_SYSTICK;
        if ((now <= end) && (now > start))
        return;
    }
  }
}

/*
 * This should work as well?

 * Here, we see if the delta has wrapped and adjust it, instead of
 * comparing separate "wrapped" and "unwrapped" regions.  It looks
 * like this results in slightly smaller code, with slightly more
 * overhead.
 */
void sysTickDelay2 (uint32_t count) __attribute__((noinline));
void sysTickDelay2 (uint32_t count)
{
  int32_t start, elapsed, reload;

  start = GET_SYSTICK;
  reload = GET_SYSTICKMAX;
  while (1) {
    elapsed = start - GET_SYSTICK;
    if (elapsed < 0)
      elapsed += reload;  // Handle wrap.
    if (elapsed >= count)
      return;
  }
}

uint32_t ticksPerUs;
uint32_t save_int;

void sleep(uint32_t t) {
//    sleep_us(t);
    sysTickDelay(t*ticksPerUs);
}
#define HISTSIZE 40
int zeros[HISTSIZE] = {0};

const uint BOOT_PIN = 9;

void reboot_callback(uint pin, uint32_t events) {
    reset_usb_boot(0,0);
}

void reboot_enable(int pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback (pin, GPIO_IRQ_LEVEL_LOW,
                                        true, reboot_callback);
}

int main() {
    void (*flash_flush_cache)(void) = (void(*)(void))rom_func_lookup(rom_table_code('F', 'C'));
    int histogram[HISTSIZE];
    int outliers, totalOutliers = 0;
    stdio_init_all();
    reboot_enable(BOOT_PIN);
    sleep_ms(10000);
    ticksPerUs = clock_get_hz(clk_sys)/1000000;
    printf("\n\nStarting\nCurrent Clock Rate is %dMHz\n", ticksPerUs);
    if (flush_cache)
        printf("Flushing cache before timed sequence\n");
    if (disable_interrupts)
        printf("Disabling interrupts around timed sequences\n");
    systick_hw->csr = 0x5;
    systick_hw->rvr = 0x00FFFFFF;  // This is the max value for the reload count
    sysTickDelay(100);
    (void) systick_hw->cvr;

    uint32_t new, old, t0, t1, delta, mymin, mymax, total, count;
    while (1) {
        outliers = 0;
        bzero(histogram, sizeof(histogram));
        mymin = 999999;
        mymax = 0;
        total = 0;
        count = 0;
        for (int t=1; t < 500; t += 1) {
            if (flush_cache)
                flash_flush_cache();  // Experiment: explains large delays?
            if (disable_interrupts)
                save_int = save_and_disable_interrupts();
            old=systick_hw->cvr;
            t0=time_us_32();
            //           sleep(t);
            //    sysTickDelay2(t);
            sysTickDelay(t);
            new=systick_hw->cvr;
            t1=time_us_32();
            delta = (old-new)-t;
            if (disable_interrupts)
                restore_interrupts(save_int);
            if (print_individual || (delta >= sizeof(histogram)/sizeof(histogram[0]))) {
                printf("  Target ticks = %d", t);
                printf("   old-new = %d  (%d delta)",old-new, delta);
                printf("   t1-t0 = %dus",t1-t0);
                count++;
            }
            total += delta;
            if (delta < mymin)
                mymin = delta;
            if (delta > mymax)
                mymax = delta;
            if (delta < sizeof(histogram)/sizeof(histogram[0])) {
                histogram[delta]++;
                printf("\n");
            } else {
                outliers++;
                totalOutliers++;
                printf(" ****\n");
            }
            sleep_ms(0000 + (rand() % 577));  // try to avoid lockstep behavior
        }
        printf("\ntiming analysis\n");
        printf("Minimum %d  Maximum %d  Average %d  Outliers %d (total %d)\n",
               mymin, mymax, total/count, outliers, totalOutliers);
        for (int i= mymin-1; i < HISTSIZE; i++) {
            printf("%02d (%3d): ", i, histogram[i]);
            for (int j=0; j < histogram[i]; j++) {
                printf("*");
                if (j > 75) break;
            }
            printf("\n");
            // Skip printing most empty lines of histogram:
            if (0 == memcmp(&histogram[i], zeros, (HISTSIZE-i)*sizeof(int)))
                break;
        }
        printf("\nSTARTING OVER\n");
        sleep_ms(5000);
    }
    return 0;
}
