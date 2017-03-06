#include <linux/reboot.h>
#include <linux/export.h>
#include <asm/sbi.h>

void (*pm_power_off)(void) = machine_power_off;
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
}

void machine_halt(void)
{
}

void machine_power_off(void)
{
  sbi_shutdown();
}
