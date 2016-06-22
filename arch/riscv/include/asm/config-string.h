#ifndef __ASM_CONFIG_STRING_H
#define __ASM_CONFIG_STRING_H

#include <linux/types.h>
#include <linux/platform_device.h>

/* Parse non-resource config parameters.
 * Success is assumed, since platform devices must export correct parameters.
 * If there is a parsing failure, there will be a kernel warning generated.
 */

/* Returns a parsed integer for the first value */
u64 config_string_u64(struct platform_device *pdev, const char *key);

/* Returns the total length of the value, writes at most maxlen bytes to dest */
int config_string_str(struct platform_device *pdev, const char *key, char *dest, int maxlen);

#endif /* __ASM_CONFIG_STRING_H */
