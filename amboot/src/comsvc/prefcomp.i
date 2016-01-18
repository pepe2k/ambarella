/* system/src/comsvc/prefcomp.i */


#ifndef __PREFCOMP_I__
#define __PREFCOMP_I__

static inline int digitvalue(char isdigit)
{
        if (isdigit >= '0' && isdigit <= '9' )
                return isdigit - '0';
        else
                return -1;
}

static inline int xdigitvalue(char isdigit)
{
        if (isdigit >= '0' && isdigit <= '9' )
                return isdigit - '0';
        if (isdigit >= 'a' && isdigit <= 'f')
                return 10 + isdigit - 'a';
        return -1;
}

static inline int ___strtou32(const char *str, u32 *value)
{
        int i;

        *value = 0;

        if (strncmp(str, "0x", 2) == 0) {
                /* hexadecimal mode */
                str += 2;

                while (*str != '\0') {
                        if ((i = xdigitvalue(*str)) < 0)
                                return -1;

                        *value = (*value << 4) | (u32) i;

                        str++;
                }
        } else {
                /* decimal mode */
                while (*str != '\0') {
                        if ((i = digitvalue(*str)) < 0)
				return -1;

                        *value = (*value * 10) + (u32) i;

                        str++;
                }
        }

        return 0;
}

static inline int set_u8_val(u8 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int set_u16_val(u16 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int set_u32_val(u32 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int set_s8_val(s8 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int set_s16_val(s16 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int set_s32_val(s32 *data, const char *val)
{
	u32 v;

	___strtou32(val, &v);
	*data = v;

	return 0;
}

static inline int get_u8_val(const u8 *data, char *val, int size_val)
{
	int rval =  snprintf(val, size_val, "0x%.2x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int get_u16_val(const u16 *data, char *val, int size_val)
{
	int rval = snprintf(val, size_val, "0x%.4x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int get_u32_val(const u32 *data, char *val, int size_val)
{
	int rval = snprintf(val, size_val, "0x%.8x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int get_s8_val(const s8 *data, char *val, int size_val)
{
	int rval =  snprintf(val, size_val, "0x%.2x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int get_s16_val(const s16 *data, char *val, int size_val)
{
	int rval = snprintf(val, size_val, "0x%.4x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int get_s32_val(const s32 *data, char *val, int size_val)
{
	int rval = snprintf(val, size_val, "0x%.8x", *data);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int set_string_val(char *data, int size_data, const char *val)
{
	strncpy(data, val, size_data);
	return 0;
}

static inline int get_string_val(const char *data, int size_data,
				 char *val, int size_val)
{
	strncpy(val, data, size_val);
	return 0;
}

static inline int get_u8_array(const u8 *data, int size_data,
			       char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "0x%.2x ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}

static inline int get_u16_array(const u16 *data, int size_data,
				char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "0x%.4x ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}

static inline int get_u32_array(const u32 *data, int size_data,
				char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "0x%.8x ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}


static inline int get_s8_array(const s8 *data, int size_data,
			       char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "%d ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}

static inline int get_s16_array(const s16 *data, int size_data,
				char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "%.d ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}

static inline int get_s32_array(const char *data, int size_data,
				char *val, int size_val)
{
	int i;
	int rval;

	for (i = 0; i < size_data && size_val > 0; i++) {
		rval = snprintf(val, size_val, "%d ", data[i]);
		if (rval < 0)
			return rval;

		size_val -= rval;
		val += rval;
	}

	return 0;
}

static inline int get_ethaddr_val(ethaddr_t *data, char *val, int size_val)
{
	int rval =  snprintf(val, size_val, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
			     data->addr[0],
			     data->addr[1],
			     data->addr[2],
			     data->addr[3],
			     data->addr[4],
			     data->addr[5]);
	if (rval < 0)
		return rval;
	return 0;
}

static inline int set_ethaddr_val(ethaddr_t *data, const char *val)
{
	unsigned char *ptr;
	int i, j;
	unsigned char v;
	unsigned char c;

	ptr = data->addr;
 	i = 0;
	do {
                j = v = 0; 

		/* We might get a semicolon here - not required. */
		if (i && (*val == ':')) {
			val++; 
		}

		do {
			c = *val;
			if (((unsigned char)(c - '0')) <= 9) {
				c -= '0';
			} else if (((unsigned char)((c | 0x20) - 'a')) <= 5) {
				c = (c | 0x20) - ('a' - 10);
			} else if (j && (c == ':' || c == 0)) {
				break;
			} else {
				return -1;
			}
			++val;
			v <<= 4;
			v += c;
		} while (++j < 2);
		*ptr++ = v;
	} while (++i < 6);

	return (int) (*val);
}

static inline int get_ipaddr_val(ipaddr_t *data, char *val, int size_val)
{
	int rval = snprintf(val, size_val, "%d.%d.%d.%d",
			    (int) (*data & 0xff),
			    (int) ((*data >> 8) & 0xff),
			    (int) ((*data >> 16) & 0xff),
			    (int) ((*data >> 24) & 0xff));
	if (rval < 0)
		return rval;
	return 0;
}

static inline int set_ipaddr_val(ipaddr_t *data, const char *val)
{
	int saw_digit, octets, ch;
	unsigned char tmp[4], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *val++) != '\0') {

		if (ch >= '0' && ch <= '9') {
			unsigned int new = *tp * 10 + (ch - '0');

			if (new > 255)
				return (0);
			*tp = new;
			if (! saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
                                return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return -1;
        } 
	if (octets < 4)
		return -2;
	memcpy(data, tmp, 4);

	return 0;
}

#endif
