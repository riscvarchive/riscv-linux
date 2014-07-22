#include <linux/reboot.h>
#include <linux/export.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
    // lock system in a loop on halt
    while(true);
}

void machine_halt(void)
{
    // lock system in a loop on halt
    while(true);
}

void machine_power_off(void)
{
    // lock system in a loop on halt
    while(true);
}
