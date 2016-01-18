
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)	(sizeof(_array)/sizeof(_array[0]))
#endif

int fd_iav;

int vout_id[] = {0, 1};
int vout_type[] = {AMBA_VOUT_SINK_TYPE_DIGITAL, AMBA_VOUT_SINK_TYPE_HDMI};
int vout_index = 1;

#define NUM_VOUT	ARRAY_SIZE(vout_id)

int vout_width[NUM_VOUT] = {1280};
int vout_height[NUM_VOUT] = {720};

int max_vout_width;
int max_vout_height;

int u_printf(const char *fmt, ...);
int v_printf(const char *fmt, ...);


int open_iav(void)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	return fd_iav;
}

int get_file_size(int fd)
{
	off_t off = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	if (off > 1*1024*1024*1024) {
		u_printf("error: file too larg\n");
		return -1;
	}
	return (int)off;
}

int open_file(const char *name, u32 *size)
{
	int fd;

	if ((fd = open(name, O_RDONLY, 0)) < 0) {
		perror(name);
		return -1;
	}

	if (size != NULL)
		*size = get_file_size(fd);

	return fd;
}


////////////////////////////////////////////////////////////////////////////////////

int ioctl_config_vout(int id, int offset_x, int offset_y, int width, int height)
{
	iav_vout_change_video_offset_t offset;
	iav_vout_change_video_size_t size;

	u_printf("set vout (id=%d) window: %d %d, %d %d\n", id, offset_x, offset_y, width, height);

	memset(&offset, 0, sizeof(offset));
	offset.vout_id = id;
	offset.specified = 1;
	offset.offset_x = offset_x;
	offset.offset_y = offset_y;
	if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &offset) < 0) {
		perror("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET");
		return -1;
	}

	memset(&size, 0, sizeof(size));
	size.vout_id = id;
	size.width = width;
	size.height = height;
	if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &size) < 0) {
		perror("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE");
		return -1;
	}

	u_printf("config vout done\n");
	return 0;
}

static u32 get_vout_mask(void)
{
	if (vout_index == NUM_VOUT) {
		return (1 << NUM_VOUT) - 1;
	} else {
		return 1 << vout_id[vout_index];
	}
}

int vout_get_sink_id(int chan, int sink_type)
{
	int					num;
	int					i;
	struct amba_vout_sink_info		sink_info;
	u32					sink_id = -1;

	num = 0;
	if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return -1;
	}
	if (num < 1) {
		u_printf("Please load vout driver!\n");
		return -1;
	}

	for (i = num - 1; i >= 0; i--) {
		sink_info.id = i;
		if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return -1;
		}

		//u_printf("sink %d is %s\n", sink_info.id, sink_info.name);

		if ((sink_info.sink_type == sink_type) &&
			(sink_info.source_id == chan))
			sink_id = sink_info.id;
	}

	//u_printf("%s: %d %d, return %d\n", __func__, chan, sink_type, sink_id);

	return sink_id;
}

int get_single_vout_info(int index, int *width, int *height)
{
	struct amba_vout_sink_info info;

	memset(&info, 0, sizeof(info));
	info.id = vout_get_sink_id(vout_id[index], vout_type[index]);
	if (info.id < 0)
		return -1;

	u_printf("vout id: %d\n", info.id);

	if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_INFO, &info) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_INFO");
		return -1;
	}

	*width = info.sink_mode.video_size.vout_width;
	*height = info.sink_mode.video_size.vout_height;

	/*u_printf("info.sink_mode.format: %d\n", info.sink_mode.format);
	if (info.sink_mode.format == AMBA_VIDEO_FORMAT_INTERLACE) {
		vout_height *= 2;
	}*/

	u_printf("vout size: %d * %d\n", *width, *height);
	if (*width == 0 || *height == 0) {
		*width = 1280;
		*height = 720;
	}

	return 0;
}

int get_vout_info(void)
{
	if (vout_index == NUM_VOUT) {
		int i;
		for (i = 0; i < NUM_VOUT; i++) {
			if (get_single_vout_info(i, vout_width + i, vout_height + i) < 0)
				return -1;
			if (max_vout_width < vout_width[i])
				max_vout_width = vout_width[i];
			if (max_vout_height < vout_height[i])
				max_vout_height = vout_height[i];
		}
	} else {
		if (get_single_vout_info(vout_index, vout_width + vout_index, vout_height + vout_index) < 0)
			return -1;

		max_vout_width = vout_width[vout_index];
		max_vout_height = vout_height[vout_index];
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////

typedef struct media_entry_s {
	char	filename[128];
	char	bmp_filename[64];
	int	udec_type;
	int	pic_width;
	int	pic_height;
	int	picf;
	struct media_entry_s *next;
} media_entry_t;

media_entry_t *media_entry_first = NULL;
media_entry_t *media_entry_last = NULL;
int udec_type_tmp = UDEC_H264;
char media_entry_bmp_filename[64];
media_entry_t *mdesc_entries[16];
int mdesc_num;

int ReadLine(FILE *file, char *line)
{
	int c;
	while ((c = fgetc(file)) != EOF) {
		if (c == '\r')
			continue;
		if (c == '\n')
			break;
		*line++ = c;
	}
	*line = 0;
	return 0;
}

char *SkipWS(char *line)
{
	while (*line == ' ' || *line == '\t')
		line++;
	return line;
}

int ParseMdecEntries(FILE *file, char *buffer, char *filename_buffer)
{
	while (1) {
		char *line = buffer;
		media_entry_t *entry;
		char *p;

		ReadLine(file, line);
		line = SkipWS(line);

		if (*line == 0 || *line == '\n' || *line == '\r' || *line == '[')
			break;

		p = filename_buffer;
		while (*line != ' ' && *line != '\t' && *line != 0)
			*p++ = *line++;
		*p = 0;

		if (mdesc_num >= sizeof(mdesc_entries)/sizeof(mdesc_entries[0])) {
			u_printf("too many mdesc entries, skip %s\n", filename_buffer);
			continue;
		}

		for (entry = media_entry_first; entry; entry = entry->next) {
			if (strcmp(entry->filename, filename_buffer) == 0) {
				mdesc_entries[mdesc_num++] = entry;
				break;
			}
		}

		if (entry == NULL) {
			u_printf("%s is not registered!\n", filename_buffer);
		} else {
			v_printf("%s : udec_type=%d, size: %d*%d\n", entry->filename,
				entry->udec_type, entry->pic_width, entry->pic_height);
		}
	}

	return 0;
}

int ParseLine(FILE *file, char *line, char *filename_buffer)
{
	char *buffer = line;
	media_entry_t *entry;
	char *p;

	line = SkipWS(line);

	if (*line == 0 || *line == '\n' || *line == '\r')
		return 0;

	if (line[0] == '/' && line[1] == '/')
		return 0;

	if (*line == '[') {
		line++;
		if (strncmp(line, "jpeg", 4) == 0) {
			udec_type_tmp = UDEC_JPEG;
			line += 4;
		}
		else if (strncmp(line, "vc1", 3) == 0) {
			udec_type_tmp = UDEC_VC1;
			line += 3;
		}
		else if (strncmp(line, "h264", 4) == 0) {
			udec_type_tmp = UDEC_H264;
			line += 4;
		}
		else if (strncmp(line, "mp12", 4) == 0) {
			udec_type_tmp = UDEC_MP12;
			line += 4;
		}
		else if (strncmp(line, "mp4h", 4) == 0) {
			udec_type_tmp = UDEC_MP4H;
			line += 4;
		}
		else if (strncmp(line, "mdec", 4) == 0) {
			ParseMdecEntries(file, buffer, filename_buffer);
		}
		else {
			u_printf("unknown udec type: %s\n", line);
			return -1;
		}
		media_entry_bmp_filename[0] = 0;
		if (*line == ':') {
			char *rstr;
			line++;
			rstr = strtok(line, "]");
			if (rstr) {
				strcpy(media_entry_bmp_filename, rstr);
			}
		}
		return 0;
	}

	// file entry
	if ((entry = malloc(sizeof(*entry))) == NULL) {
		perror("malloc");
		return -1;
	}

	entry->udec_type = udec_type_tmp;
	entry->picf = 0;

	strcpy(entry->bmp_filename, media_entry_bmp_filename);

	p = entry->filename;
	while (*line != ' ' && *line != '\t' && *line != 0)
		*p++ = *line++;
	*p = 0;

	line = SkipWS(line);
	if (*line == 0) {
		u_printf("resolution not specified for %s\n", entry->filename);
		return -1;
	}
	sscanf(line, "%d*%d", &entry->pic_width, &entry->pic_height);

	while (1) {
		int c = *line;
		if (c == ',' || c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0)
			break;
		line++;
	}

	if (*line == ',') {
		line++;
		entry->picf = *line - '0';
		//u_printf("%s: picf = %d\n", entry->filename, entry->picf);
	}

	entry->next = NULL;

	if (media_entry_first == NULL) {
		media_entry_first = entry;
		media_entry_last = entry;
	}
	else {
		media_entry_last->next = entry;
		media_entry_last = entry;
	}

	//u_printf("%s: %d %d\n", entry->filename, entry->pic_width, entry->pic_height);

	return 0;
}

char *GetFileNameOnly(char *filename)
{
	int len = strlen(filename);
	char *ptr = filename + len;
	while (*ptr != '/') ptr--;
	return ptr;
}

int ReadMediaEntries(void)
{
	char media_filename[256];
	char *first_file;
	char line[256];
	FILE *file;

	first_file = file_list[0];
	if (first_file[0] != 0) {
		char *fname = GetFileNameOnly(first_file);
		memset(pathname, 0, sizeof(pathname));
		strncpy(pathname, first_file, fname - first_file);
	}

	sprintf(media_filename, "%s/media.txt", pathname);

	if ((file = fopen(media_filename, "r")) == NULL) {
		u_printf("Cannot open %s\n", media_filename);
		return -1;
	}

	while (!feof(file)) {
		ReadLine(file, line);
		if (ParseLine(file, line, media_filename) < 0)
			return -1;
	}

	fclose(file);
	return 0;
}

void GetFileEntry(media_entry_t *entry, int file_index)
{
	char *file_name = file_list[file_index];
	sprintf(file_name, "%s/%s", pathname, entry->filename);
	sprintf(current_bmp_filename, "%s/%s", pathname, entry->bmp_filename);
	pic_width = entry->pic_width;
	pic_height = entry->pic_height;
	picf = entry->picf;
	u_printf("%s: %d*%d\n", file_name, pic_width, pic_height);
}

#ifdef DECODE_MDEC
int GetMdecFileInfo(void)
{
	media_entry_t *entry = mdesc_entries[mdec_pindex];
	char *file_name = file_list[0];
	sprintf(file_name, "%s/%s", pathname, entry->filename);
	udec_type = entry->udec_type;
	pic_width = entry->pic_width;
	pic_height = entry->pic_height;
	picf = entry->picf;
	v_printf("%s: %d*%d\n", file_name, pic_width, pic_height);
	return 0;
}
#endif

// udec_type file_index => filename
int GetMediaFileInfo(int file_index)
{
	int index;
	media_entry_t *entry;
	char *file_name = file_list[file_index];

	if (file_name[0] != 0) {
		char *fname = GetFileNameOnly(file_name) + 1;
		entry = media_entry_first;
		for (; entry; entry = entry->next) {
			if (strcmp(fname, entry->filename) == 0) {
				udec_type = entry->udec_type;
				GetFileEntry(entry, file_index);
				break;
			}
		}
		if (entry == NULL) {
			u_printf("%s is not registered in media.txt!\n", file_name);
			return -1;
		}
		return 0;
	}

	index = 0;
	entry = media_entry_first;
	for (; entry; entry = entry->next) {
		if (entry->udec_type == udec_type) {
			if (index == file_index) {
				GetFileEntry(entry, file_index);
				return 0;
			}
			index++;
		}
	}

	u_printf("type %d index %d not found!\n", udec_type, file_index);
	return -1;
}

long long get_tick(void)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	static struct timeval start_tv;
	static int started;
	struct timeval tv;
	long long rval;

	pthread_mutex_lock(&mutex);

	if (!started) {
		started = 1;
		gettimeofday(&start_tv, NULL);
		rval = 0;
	} else {
		gettimeofday(&tv, NULL);
		rval = (1000000LL * (tv.tv_sec - start_tv.tv_sec) + (tv.tv_usec - start_tv.tv_usec)) / 1000;
	}

	pthread_mutex_unlock(&mutex);

	return rval;
}


u8 *mfile_ptr_start = NULL;
u8 *mfile_ptr_end = NULL;
u8 *mfile_ptr = NULL;
int mfile_size = 0;

int Media_InitForFile(int fd, int fsize)
{
	mfile_ptr_start = (u8*)mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd, 0);
	if (mfile_ptr_start == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	mfile_ptr_end = mfile_ptr_start + fsize;
	mfile_size = fsize;
	return 0;
}

void Media_Reset(void)
{
	mfile_ptr = mfile_ptr_start;
}

int Media_EOF(void)
{
	return mfile_ptr == mfile_ptr_end;
}

int Media_Release(void)
{
	if (munmap(mfile_ptr_start, mfile_size) < 0) {
		perror("munmap");
		return -1;
	}
	return 0;
}

int Media_GetFrameSize(int pic_code1, int pic_code2, int code1, int code2)
{
	u8 *ptr = mfile_ptr;
	int count = 2;

	while (ptr < mfile_ptr_end) {
		int state = 0;
		while (1) {
			switch (state) {
			case 0:		//
				if (*ptr == 0) state = 1;
				//else state = 0;
				break;

			case 1:		// 00
				if (*ptr == 0) state = 2;
				else state = 0;
				break;

			case 2:		// 00 00
				if (*ptr == 1) state = 3;
				else if (*ptr == 0) ; //state = 2;
				else state = 0;
				break;

			case 3:		// 00 00 01
				if (*ptr == pic_code1 || *ptr == pic_code2) {
					//v_printf("start code found at %d\n", ptr - 3 - mfile_ptr_start);
					if (--count == 0) return ptr - 3 - mfile_ptr;
					else state = 0;
				}
				else if (count == 1 && (*ptr == code1 || *ptr == code2)) {
					return ptr - 3 - mfile_ptr;
				}
				else if (*ptr == 0x00) {
					state = 1;
				}
				else {
					state = 0;
				}
				break;
			}

			if (++ptr == mfile_ptr_end)
				break;
		}
	}

	if (count == 2) {
		u_printf("start code not found, start_word = %x %x %x %x\n",
			mfile_ptr[0], mfile_ptr[1], mfile_ptr[2], mfile_ptr[3]);
		exit(0);
	}

	v_printf("::: last frame read :::\n");
	return mfile_ptr_end - mfile_ptr;
}

int VC1_GetFrameSize(void)
{
	return Media_GetFrameSize(0x0D, 0x0C, 0x0F, 0x0E);
}

int MP12_GetFrameSize(void)
{
	// 00 - picture header
	// B3 - sequence header
	// B8 - GOP header
	return Media_GetFrameSize(0x00, 0x00, 0xB3, 0xB8);
}

int MP4H_GetFrameSize(void)
{
	return Media_GetFrameSize(0xB6, 0xB6, 0xB6, 0xB6);
}

int H264_GetFrameSize(void)
{
	u8 *ptr = mfile_ptr;
	int count = 2;
	int nal_type;

	while (ptr < mfile_ptr_end) {
		int state = 0;
		while (1) {
			switch (state) {
			case 0:		//
				if (*ptr == 0) state = 1;
				//else state = 0;
				break;

			case 1:		// 00
				if (*ptr == 0) state = 2;
				else state = 0;
				break;

			case 2:		// 00 00
				if (*ptr == 1) state = 3;
				else if (*ptr == 0) ; //state = 2;
				else state = 0;
				break;

			case 3:	// 00 00 01
				nal_type = (*ptr) & 0x1F;
				if (nal_type >= 1 && nal_type <= 5) {
					if (--count == 0) return ptr - 3 - mfile_ptr;
					else state = 0;
				}
				else if (count == 1 && nal_type >= 6 && nal_type <= 9) {
					// left SPS and PPS be prefix
					return ptr - 3 - mfile_ptr;
				}
				else if (*ptr == 0x00) {
					state = 1;
				}
				else {
					state = 0;
				}
				break;
			}

			if (++ptr == mfile_ptr_end)
				break;
		}
	}

	if (count == 2) {
		u_printf("start code not found, start_word = %x %x %x %x\n",
			mfile_ptr[0], mfile_ptr[1], mfile_ptr[2], mfile_ptr[3]);
		exit(0);
	}

	v_printf("::: last frame read\n");
	return mfile_ptr_end - mfile_ptr;
}

void frame_too_large(void)
{
	u_printf("Frame too large!!!\n");
	exit(-1);
}

void copy_to_fifo(u8 *fifo_start, u32 fifo_size, u8 *fifo_ptr,
	const u8 *source, u32 size, int is_eos)
{
	const u8 *end = fifo_start + fifo_size;
	static int wrap_count;

	if (udec_type == UDEC_JPEG) {
		if (size >= (u32)fifo_size)
			frame_too_large();
	} else {
		if (size >= (u32)fifo_size)
			frame_too_large();
	}

	if (fifo_ptr + size <= end)
		memcpy(fifo_ptr, source, size);
	else {
		u32 tocopy = end - fifo_ptr;
		if (tocopy > 0) {
			memcpy(fifo_ptr, source, tocopy);
			source += tocopy;
		}
		tocopy = size - tocopy;
		if (tocopy > 0)
			memcpy(fifo_start, source, tocopy);

		wrap_count++;
		if (is_eos)
			u_printf(" *** EOS wrap around %d, %d+%d\n", wrap_count, size - tocopy, tocopy);
		else
			u_printf(" *** fifo wrap around %d, %d+%d\n", wrap_count, size - tocopy, tocopy);
	}
}

u32 Media_ReadFrame(u8 *fifo_start, u32 fifo_size, u8 *fifo_ptr)
{
	int fsize;

	switch (udec_type) {
	case UDEC_VC1: fsize = VC1_GetFrameSize(); break;
	case UDEC_MP12: fsize = MP12_GetFrameSize(); break;
	case UDEC_MP4H: fsize = MP4H_GetFrameSize(); break;
	case UDEC_H264: fsize = H264_GetFrameSize(); break;
	case UDEC_JPEG: fsize = mfile_size; break;
	default:
		u_printf("todo\n");
		exit(-1);
	}

	copy_to_fifo(fifo_start, fifo_size, fifo_ptr, mfile_ptr, fsize, 0);
	mfile_ptr += fsize;

	return fsize;
}

u32 Media_AppendEOS(u8 *fifo_start, u32 fifo_size, u8 *fifo_ptr)
{
	switch (udec_type) {
	case UDEC_H264: {
			static const u8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
			copy_to_fifo(fifo_start, fifo_size, fifo_ptr, eos, sizeof(eos), 1);
			return sizeof(eos);
		}
		break;

	case UDEC_VC1: {
			static const u8 eos[] = {0, 0, 0x01, 0xA};
			copy_to_fifo(fifo_start, fifo_size, fifo_ptr, eos, sizeof(eos), 1);
			return sizeof(eos);
		}
		break;

	case UDEC_MP12: {
			static const u8 eos[] = {0, 0, 0x01, 0xB7};
			copy_to_fifo(fifo_start, fifo_size, fifo_ptr, eos, sizeof(eos), 1);
			return sizeof(eos);
		}
		break;

	case UDEC_MP4H: {
			static const u8 eos[] = {0, 0, 0x01, 0xB1};
			copy_to_fifo(fifo_start, fifo_size, fifo_ptr, eos, sizeof(eos), 1);
			return sizeof(eos);
		}
		break;

	case UDEC_JPEG:
		return 0;

	default:
		u_printf("AppendEOS: udec type %d not implemented!\n", udec_type);
		return 0;
	}
}

