#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "hardware/structs/clocks.h"

#define GET_SYSTICK systick_hw->cvr
#define GET_SYSTICKMAX systick_hw->rvr
 
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

void sysTickDelay (uint32_t count) __attribute__((noinline));
void sysTickDelay (uint32_t count)
{
  uint32_t reload = GET_SYSTICKMAX;
  int32_t start, end, now;

//  if (count == 0)
//    return;

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

//  if (count == 0)
//    return;

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

void sleep(uint32_t t) {
//    sleep_us(t);
    sysTickDelay(t*ticksPerUs);
}


int main() {
    stdio_init_all();
    sleep_ms(10000);
    ticksPerUs = clock_get_hz(clk_sys)/1000000;
    printf("\n\nStarting\nCurrent Clock Rate is %dMHz\n", ticksPerUs);
    systick_hw->csr = 0x5;
    systick_hw->rvr = 0x00FFFFFF;  // This is the max value for the reload count
    sysTickDelay(100);
    (void) systick_hw->cvr;

    uint32_t new, old, t0, t1;
    while (1) {
        for (int t=10; t < 20; t += 1) {
            old=systick_hw->cvr;
            t0=time_us_32();
            //           sleep(t);
            //    sysTickDelay2(t);
            sysTickDelay(t);
            new=systick_hw->cvr;
            t1=time_us_32();
            printf("\n Target ticks = %d\n", t);
            printf("   old-new=%d  (%d delta)\n",old-new, (old-new)-t);
            printf("   t1-t0=    %dus\n",t1-t0);
            sleep_ms(1000 + (rand() % 37));  // try to avoid lockstep behavior
        }
        printf("\nSTARTING OVER\n");
        sleep_ms(5000);
    }
    return 0;
}
