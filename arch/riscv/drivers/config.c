/*
 * The RISC-V Platform requires only one device: the config ROM
 *
 * Copyright (C) 2016 SiFive Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#include <linux/io.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

static ssize_t config_read(struct file *filp, struct kobject *kobj,
                           struct bin_attribute *attr,
                           char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);

	memcpy_fromio(buf, pdev->archdata.config_start + off, count);
	return count;
}

static BIN_ATTR_RO(config, 0);
static void enable_config_attribute(struct platform_device *pdev)
{
	const char *end = pdev->archdata.config_end;
	const char *start = pdev->archdata.config_start;
	int ret;

	if (!end || !start) return;

	memcpy(&pdev->archdata.config, &bin_attr_config, sizeof(struct bin_attribute));
	pdev->archdata.config.size = end - start;

	ret = sysfs_create_bin_file(&pdev->dev.kobj, &pdev->archdata.config);
	if (ret)
		dev_warn(&pdev->dev, "Cannot create sysfs bin_attr_config; %d\n", ret);
}

static const char *skip_whitespace(const char *str)
{
	while (*str && *str <= ' ')
		++str;
	return str;
}

static const char *skip_newline(const char *str)
{
	while (*str && *str <= ' ')
		if (*str++ == '\n')
			break;
	return str;
}

static const char *skip_string(const char *str)
{
	while (*str && *str++ != '"');
	return str;
}

static const char *skip_key(const char *str)
{
	while (*str >= 35 && *str <= 122 && *str != ';')
		++str;
	return str;
}

static int is_hex(char c)
{
	return (c >= '0' && c <= '9') ||
	       (c >= 'a' && c <= 'f') ||
	       (c >= 'A' && c <= 'F');
}

static int parse_hex(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else {
		return c - 'A' + 10;
	}
}

static const char *parse_u64_hex(const char *str, uint64_t *output)
{
	uint64_t res = 0;
	for (; *str; ++str) {
		if (is_hex(*str)) {
			if (((res << 4) >> 4) != res) break;
			res = (res << 4) + parse_hex(*str);
		} else if (*str != '_') {
			break;
		}
	}
	*output = res;
	return str;
}

static int is_dec(char c)
{
	return (c >= '0' && c <= '9');
}

static int parse_dec(char c)
{
	return c - '0';
}

static const char *parse_u64_dec(const char *str, uint64_t *output)
{
	uint64_t res = 0;
	for (; *str; ++str) {
		if (is_dec(*str)) {
			if (((res * 10) / 10) != res) break;
			res = (res * 10) + parse_dec(*str);
		} else if (*str != '_') {
			break;
		}
	}
	*output = res;
	return str;
}

static const char *parse_u64(const char *str, uint64_t *output)
{
	uint64_t res;
	int neg = 0;
	
	if (str[0] == '-') {
		++str;
		neg = 1;
	}
	
	if (str[0] == '0' && str[1] == 'x') {
		str = parse_u64_hex(str+2, &res);
	} else {
		str = parse_u64_dec(str, &res);
	}
	
	if (neg) res = -res;
	*output = res;
	return str;
}

static int parse_quoted_string(const char *str, char *output, int maxlen)
{
	const char *s;
	int out;

	out = 0;
	for (s = str; *s && *s != '"'; ++s) {
		char ch;
		if (*s == '\\' && s[1] == 'x' && is_hex(s[2])) {
			ch = parse_hex(s[2]);
			if (is_hex(s[3])) {
				ch = (ch << 4) + parse_hex(s[3]);
				++s;
			}
			s += 2;
		} else {
			ch = *s;
		}

		++out;
		if (maxlen) {
			*output++ = ch;
			--maxlen;
		}
	}

	if (maxlen) *output = 0;
	++out;

	return out;
}

static int parse_string(const char *str, char *output, int maxlen)
{
	const char *s;
	int out;

	if (*str == '"') return parse_quoted_string(str+1, output, maxlen);

	out = 0;
	for (s = str; *s >= 35 && *s <= 122 && *s != ';'; ++s) {
		++out;
		if (maxlen) {
			*output++ = *s;
			--maxlen;
		}
	}

	if (maxlen) *output = 0;
	++out;

	return out;
}

static const char *find_key(const char *start, const char *end, const char *name, int take)
{
	int len = strlen(name);

	/* keep this function in sync with parse_config_string */
	start = skip_whitespace(start);
	while (end - start > 0 && *start != '}') {
		const char *key_start = start;
		const char *key_end = start = skip_key(start);
		int key_len = key_end - key_start;
		int min_len = key_len < len ? key_len : len;

		/* Either a complete match, or key+"." is a prefix of name */
		int substr_match = strncmp(name, key_start, min_len) == 0;
		int full_match = substr_match && key_len == len;
		int prefix_match = substr_match && (key_len < len && name[key_len] == '.');

		start = skip_whitespace(start);
		while (end - start > 0 && *start != ';') {
			if (*start == '"') {
				if (take && full_match) return start;
				start = skip_string(start+1);
			} else if (*start == '{') {
				if (take && prefix_match) {
					return find_key(start+1, end, name+key_len+1, 1);
				} else {
					start = find_key(start+1, end, "", 0);
				}
				if (*start == '}') ++start;
			} else {
				if (take && full_match) return start;
				start = skip_key(start+1);
			}
			start = skip_whitespace(start);
		}
		if (*start == ';') ++start;
		start = skip_whitespace(start);
	}

	return start;
}

u64 config_string_u64(struct platform_device *pdev, const char *key)
{
	const char *start = pdev->archdata.config_start;
	const char *end = pdev->archdata.config_end;
	const char *value;
	const char *tail;
	const char *rest;
	u64 result;

	if (!start || !end) return 0;

	value = find_key(start, end, key, 1);
	if (end - value <= 0) {
		dev_err(&pdev->dev, "Could not find u64 config string value for '%s'\n", key);
		return 0;
	}

	tail = skip_key(value);
	rest = parse_u64(value, &result);
	if (tail != rest) {
		dev_err(&pdev->dev, "Could not parse u64 config string value '%*pE' for '%s'\n",
			(int)(tail - value), value, key);
		return 0;
	}

	return result;
}
EXPORT_SYMBOL_GPL(config_string_u64)

int config_string_str(struct platform_device *pdev, const char *key, char *dest, int maxlen)
{
	const char *start = pdev->archdata.config_start;
	const char *end = pdev->archdata.config_end;
	const char *value;

	if (!start || !end) goto empty_out;

	value = find_key(start, end, key, 1);
	if (end - value <= 0) {
		dev_err(&pdev->dev, "Could not find config string value for '%s'\n", key);
		goto empty_out;
	}

	return parse_string(value, dest, maxlen);
empty_out:
	if (maxlen) *dest = 0;
	return 0;
}
EXPORT_SYMBOL_GPL(config_string_str)

/* Keywords for linux resources */
static const char interface[] = "interface";
static const char irq[] = "irq";
static const char bus[] = "bus";
static const char add[] = "add";

#define HAVE_DEVICES	1
#define HAVE_NAMES	2
#define HAVE_RESOURCES	2

struct parser_state {
	struct parser_state *parent;
	int pass;
	char* roots;
	const char *config;
	struct platform_device *devices;
	char *names;
	const char *key_start;
	const char *key_end;
};

static int parser_key_eq(const struct parser_state *state, const char *key)
{
	const char *start = state->key_start;
	const char *end = state->key_end;
	return end-start == strlen(key) && !memcmp(start, key, end-start);
}

/* parent must be != 0 */
static void config_path_helper(struct parser_state *state, const struct parser_state *parent)
{
	int len = parent->key_end - parent->key_start;

	if (parent->parent) config_path_helper(state, parent->parent);

	if (state->pass >= HAVE_NAMES) {
		memcpy(state->names, parent->key_start, len);
		state->names[len] = '.';
	}
	state->names += len + 1;
}

static const char *config_path(struct parser_state *state, const struct parser_state *parent)
{
	const char *output;

	if (!parent) return ".";

	output = state->names;
	config_path_helper(state, parent);
	if (state->pass >= HAVE_NAMES) state->names[-1] = 0;
	return output;
}

static void parse_interface(struct parser_state *state, struct platform_device *target)
{
	int len = (state->pass >= HAVE_NAMES) ? 65536 : 0;
	if (target) target->driver_override = state->names;
	state->names += parse_string(state->config, state->names, len);
}

static void parse_range(
	struct parser_state *state,
	const struct parser_state *parent,
	unsigned long flags,
	struct platform_device *target)
{
	int last_pass;
	struct resource *resource;
	uint64_t key, value;
	const char *value_start;
	const char *value_tail;
	const char *value_end;
	const char *key_end;
	const char *name;

	last_pass = target && target->resource;
	value_start = state->config;
	value_tail = skip_key(value_start);
	value_end = parse_u64(value_start, &value);
	key_end = parse_u64(state->key_start, &key);

	if (key_end != state->key_end) {
		if (last_pass) {
			printk(KERN_ERR "platform: ignoring invalid resource key '%*pE'\n",
				(int)(state->key_end - state->key_start), state->key_start);
		}
		return;
	}

	if (value_end != value_tail) {
		if (last_pass) {
			printk(KERN_ERR "platform: ignoring invalid resource value '%*pE'\n",
				(int)(value_tail - value_start), value_start);
		}
		return;
	}

	if (!target) return;

	name = config_path(state, parent->parent);
	if (target->resource) {
		resource = target->resource + target->num_resources;
		memset(resource, 0, sizeof(*resource));
		resource->name  = *name?name:".";
		resource->flags = flags;
		resource->start = key;
		resource->end   = value;
	}
	
	++target->num_resources;
}

static void parse_single(
	struct parser_state *state,
	const struct parser_state *parent,
	unsigned long flags,
	struct platform_device *target)
{
	int last_pass;
	struct resource *resource;
	uint64_t value;
	const char *value_start;
	const char *value_tail;
	const char *value_end;
	const char *name;

	last_pass = target && target->resource;
	value_start = state->config;
	value_tail = skip_key(value_start);
	value_end = parse_u64(value_start, &value);

	if (value_end != value_tail) {
		if (last_pass) {
			printk(KERN_ERR "platform: ignoring invalid resource value '%*pE'\n",
				(int)(value_tail - value_start), value_start);
		}
		return;
	}

	if (!target) return;

	name = config_path(state, parent);
	if (target->resource) {
		resource = target->resource + target->num_resources;
		memset(resource, 0, sizeof(*resource));
		resource->name  = *name?name:".";
		resource->flags = flags;
		resource->start = value;
		resource->end   = value;
	}
	
	++target->num_resources;
}

static struct parser_state parse_config_string(
	struct parser_state state,
	struct parser_state *parent,
	unsigned long parent_flags,
	struct platform_device *target)
{
	/* keep this function in sync with find_key */
	char *am_root = state.roots++;

	if (*am_root) {
		if (state.pass >= HAVE_DEVICES) {
			target = state.devices;
			target->name = config_path(&state, parent);
			target->id = -1;
			target->num_resources = 0;
			target->archdata.config_start = state.config;
			target->archdata.config_end = state.config;
		}
		++state.devices;
		parent = 0;
	}
	
	state.config = skip_whitespace(state.config);
	while (*state.config && *state.config != '}') {
		int iface;
		unsigned long flags = 0;
		state.key_start = state.config;
		state.config = skip_key(state.config);
		state.key_end = state.config;
		
		iface = parser_key_eq(&state, "interface");
		if (iface) *am_root = 1;
		if (parser_key_eq(&state, "irq")) flags = IORESOURCE_IRQ;
		if (parser_key_eq(&state, "bus")) flags = IORESOURCE_BUS;
		if (parser_key_eq(&state, "mem")) flags = IORESOURCE_MEM;
		
		state.config = skip_whitespace(state.config);
		while (*state.config && *state.config != ';') {
			if (*state.config == '"') {
				if (iface) parse_interface(&state, target);
				state.config = skip_string(state.config+1);
			} else if (*state.config == '{') {
				state.config = skip_newline(state.config+1);
				state.parent = parent;
				state = parse_config_string(state, &state, flags, target);
				if (*state.config == '}') ++state.config;
			} else {
				if (iface)
					parse_interface(&state, target);
				if (parent_flags)
					parse_range(&state, parent, parent_flags, target);
				if (flags)
					parse_single(&state, parent, flags, target);
				state.config = skip_key(state.config+1);
			}
			iface = 0;
			state.config = skip_whitespace(state.config);
		}
		if (*state.config == ';') ++state.config;
		if (*am_root && target && state.pass >= HAVE_DEVICES) {
			target->archdata.config_end = skip_newline(state.config);
		}
		state.config = skip_whitespace(state.config);
	}
	
	return state;
}

static void *config_find_devices(const char *config)
{
	struct parser_state state;
	char *roots, *r;
	int num_devices, i;
	void *mem;
	struct platform_device *devices;
	int num_resources;
	struct resource *resources;
	char *names;
	int size_devices, size_resources, size_names;
	int off_devices, off_resources, off_names, off_end;

	roots = kzalloc(strlen(config), GFP_KERNEL);
	BUG_ON(roots == 0);

	// Pass 0: detect roots
	state.pass = 0;
	state.roots = roots;
	state.config = config;
	state = parse_config_string(state, 0, 0, 0);

	num_devices = 0;
	for (r = roots; r != state.roots; ++r)
		num_devices += *r;

	size_devices = sizeof(*devices) * num_devices;
	off_devices = DIV_ROUND_UP(sizeof(u32), sizeof(*devices)) * sizeof(*devices);

	mem = kzalloc(size_devices + off_devices, GFP_KERNEL);
	BUG_ON(mem == 0);
	*(u32*)mem = num_devices;
	devices = mem + off_devices;

	// Pass 1: detect resources and names
	state.pass = 1;
	state.roots = roots;
	state.config = config;
	state.devices = devices;
	state.names = 0;
	state = parse_config_string(state, 0, 0, 0);

	num_resources = 0;
	for (i = 0; i < num_devices; ++i)
		num_resources += devices[i].num_resources;
	size_resources = sizeof(*resources) * num_resources;
	size_names = state.names - (char*)0;

	off_resources = DIV_ROUND_UP(off_devices + size_devices, sizeof(*resources)) * sizeof(*resources);
	off_names = off_resources + size_resources;
	off_end = DIV_ROUND_UP(off_names + size_names, PAGE_SIZE) * PAGE_SIZE;

	mem = krealloc(mem, off_end, GFP_KERNEL);
	BUG_ON(mem == 0);
	devices = mem + off_devices;
	resources = mem + off_resources;
	names = mem + off_names;

	num_resources = 0;
	for (i = 0; i < num_devices; ++i) {
		devices[i].resource = resources + num_resources;
		num_resources += devices[i].num_resources;
	}

	// Pass 2: write resources and names
	state.pass = 2;
	state.roots = roots;
	state.config = config;
	state.devices = devices;
	state.names = names;
	state = parse_config_string(state, 0, 0, 0);

	// Done!
	kfree(roots);

	// Register devices
	for (i = 0; i < num_devices; ++i) {
		platform_device_register(devices + i);
		enable_config_attribute(devices + i);
	}

	return mem;
}

static int config_string_probe(struct platform_device *pdev)
{
	struct resource *res;
	void __iomem *mem;
	void *kdata;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		return -ENOENT;
	}

	mem = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mem)) {
		dev_err(&pdev->dev, "Cannot map resources %ld\n", PTR_ERR(mem));
		return -ENOENT;
	}

	/* Fixup our missing config attribute */
	pdev->archdata.config_start = mem;
	pdev->archdata.config_end = mem + resource_size(res) - 1; // remove null terminator
	enable_config_attribute(pdev);

	kdata = config_find_devices(mem);
	dev_set_drvdata(&pdev->dev, kdata);

	return 0;
}

static int config_string_remove(struct platform_device *pdev)
{
	void *kdata = dev_get_drvdata(&pdev->dev);
	u32 i, num_devices = *(u32*)kdata;
	struct platform_device *devices = kdata;
	
	++devices;
	for (i = 0; i < num_devices; ++i) {
		platform_device_unregister(devices + i);
	}

	kfree(kdata);

	return 0;
}

static struct platform_driver config_string_driver = {
	.probe		= config_string_probe,
	.remove		= config_string_remove,
	.driver		= {
		.name	= "config-string",
	},
};

static int __init riscv_config_init(void)
{
	platform_driver_register(&config_string_driver);
	return 0;
}

arch_initcall(riscv_config_init)
