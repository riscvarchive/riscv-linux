#include <linux/reboot.h>
#include <linux/export.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
}

void machine_halt(void)
{
}

void machine_power_off(void)
{
}
