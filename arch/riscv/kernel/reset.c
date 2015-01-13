#include <linux/reboot.h>
#include <linux/export.h>
#include <asm/htif.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
  machine_power_off();
}

void machine_halt(void)
{
  machine_power_off();
}

static uint64_t exitbuf[8] __attribute__((aligned(64)));

void machine_power_off(void)
{
  exitbuf[0] = 93; // exit
  exitbuf[1] = 0;
  mb();
  htif_tohost(0, 0, __pa(&exitbuf[0]));
  while (1) { }
}
