
#define DSP_PAGE_MASK	(GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO)
#define dsp_mm_copy_msg(_member, _msg)	\
	do { \
		typeof(G_dsp_obj->_member) *dsp_msg = (void*)(_msg); \
		/*dsp_save_msg(dsp_msg, sizeof(*dsp_msg), port);*/ \
		G_dsp_obj->_member = *dsp_msg; \
		G_dsp_obj->pair_callback_id = dsp_msg->callback_id; \
		G_dsp_obj->pair_error_code = dsp_msg->error_code; \
		pair_wakeup(); \
	} while (0)

#define dsp_mm_copy_msg2(_member, _msg)	\
	do { \
		typeof(G_dsp_obj->_member) *dsp_msg = (void*)(_msg); \
		/*dsp_save_msg(dsp_msg, sizeof(*dsp_msg), port);*/ \
		G_dsp_obj->_member = *dsp_msg; \
		G_dsp_obj->pair_callback_id = dsp_msg->callback_id; \
		G_dsp_obj->pair_error_code = 0; \
		pair_wakeup(); \
	} while (0)

static inline void pair_wakeup(void)
{
	up(&G_dsp_obj->pair_sem);
}

static inline int pair_error_code(void)
{
	return G_dsp_obj->pair_error_code;
}

static void handle_memm_msg(void *context, unsigned cat, DSP_MSG *msg, int port)
{
//	iav_dbg_printk("=== CAT_MEMM : 0x%x ===\n", msg->msg_code);

	switch (msg->msg_code) {
	case MSG_MEMM_CONFIG_FRM_BUF_POOL:
		dsp_mm_copy_msg(memm_config_frm_buf_pool_msg, msg);
		break;

	case MSG_MEMM_QUERY_DSP_SPACE_SIZE:
		dsp_mm_copy_msg2(memm_query_dsp_space_size_msg, msg);
		break;

	case MSG_MEMM_SET_DSP_DRAM_SPACE:
		dsp_mm_copy_msg(memm_set_dsp_dram_space_msg, msg);
		break;

	case MSG_MEMM_RESET_DSP_DRAM_SPACE:
		dsp_mm_copy_msg(memm_reset_dsp_dram_space_msg, msg);
		break;

	case MSG_MEMM_CREATE_FRM_BUF_POOL:
		dsp_mm_copy_msg(memm_create_frm_buf_pool_msg, msg);
		break;

	case MSG_MEMM_GET_FRM_BUF_POOL_INFO:
		dsp_mm_copy_msg2(memm_get_frm_buf_pool_info_msg, msg);
		break;

	case MSG_MEMM_UPDATE_FRM_BUF_POOL_CONFIG:
		dsp_mm_copy_msg(memm_update_frm_buf_pool_config_msg, msg);
		break;

	case MSG_MEMM_REQ_FRM_BUF:
		dsp_mm_copy_msg(memm_req_frm_buf_msg, msg);
		break;

	case MSG_MEMM_REQ_RING_BUF:
		break;

	case MSG_MEMM_CREATE_THUMBNAIL_BUF_POOL:
		break;

	case MSG_MEMM_GET_FRM_BUF_INFO:
		break;

	case MSG_MEMM_GET_FREE_FRM_BUF_NUM:
		break;

	case MSG_MEMM_GET_FREE_SPACE_SIZE:
		break;

	default:
		iav_dbg_printk("unknown msg, cat = %d, code = 0x%x\n", cat, msg->msg_code);
	}
}

int dsp_mm_set_dram_space(dsp_vm_space_t *vms)
{
	MEMM_SET_DSP_DRAM_SPACE_CMD dsp_cmd;

	if (!SPACE_ID_VALID(vms->space_id) || vms->pfn == NULL) {
		iav_dbg_printk("bad vms, space_id=%d, pfn=0x%x\n", vms->space_id, (u32)vms->pfn);
		return -EINVAL;
	}

	clean_d_cache(vms->pfn, vms->nr_pages * sizeof(unsigned long));

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_MEMM_SET_DSP_DRAM_SPACE;
	dsp_cmd.callback_id = CMD_MEMM_SET_DSP_DRAM_SPACE;
	dsp_cmd.dram_space_addr = vms->space_id << DSP_SPACE_ID_SHIFT;
	dsp_cmd.dram_space_size = vms->nr_pages << PAGE_SHIFT;
	dsp_cmd.page_tb_addr = VIRT_TO_DSP(vms->pfn);
	dsp_cmd.dram_space_id = vms->space_id;

	__dsp_issue_cmd_pair_msg(&dsp_cmd, sizeof(dsp_cmd), 0);

	return pair_error_code();
}

int dsp_mm_reset_dram_space(dsp_vm_space_t *vms)
{
	MEMM_RESET_DSP_DRAM_SPACE_CMD dsp_cmd;

	if (!SPACE_ID_VALID(vms->space_id) || vms->pfn == NULL) {
		iav_dbg_printk("bad vms, space_id=%d, pfn=0x%x\n", vms->space_id, (u32)vms->pfn);
		return -EINVAL;
	}

	clean_d_cache(vms->pfn, vms->nr_pages * sizeof(unsigned long));

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = CMD_MEMM_RESET_DSP_DRAM_SPACE;
	dsp_cmd.callback_id = CMD_MEMM_RESET_DSP_DRAM_SPACE;
	dsp_cmd.dram_space_id = vms->space_id;

	__dsp_issue_cmd_pair_msg(&dsp_cmd, sizeof(dsp_cmd), 0);

	return pair_error_code();
}

void dsp_mm_config_frm_buf_pool(MEMM_CONFIG_FRM_BUF_POOL_CMD *dsp_cmd, MEMM_CONFIG_FRM_BUF_POOL_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 1);
	*dsp_msg = G_dsp_obj->memm_config_frm_buf_pool_msg;
}
EXPORT_SYMBOL(dsp_mm_config_frm_buf_pool);

void dsp_mm_query_dsp_space_size(MEMM_QUERY_DSP_SPACE_SIZE_CMD *dsp_cmd, MEMM_QUERY_DSP_SPACE_SIZE_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 1);
	*dsp_msg = G_dsp_obj->memm_query_dsp_space_size_msg;
}

int dsp_mm_reset_dsp_dram_space(MEMM_RESET_DSP_DRAM_SPACE_CMD *dsp_cmd)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 1);
	return pair_error_code();
}

void dsp_mm_create_frm_buf_pool(MEMM_CREATE_FRM_BUF_POOL_CMD *dsp_cmd, MEMM_CREATE_FRM_BUF_POOL_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 0);
	*dsp_msg = G_dsp_obj->memm_create_frm_buf_pool_msg;
}

void dsp_mm_get_frm_buf_pool_info(MEMM_GET_FRM_BUF_POOL_INFO_CMD*dsp_cmd, MEMM_GET_FRM_BUF_POOL_INFO_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 0);
	*dsp_msg = G_dsp_obj->memm_get_frm_buf_pool_info_msg;
}

void dsp_mm_update_frm_buf_pool_config(MEMM_UPDATE_FRM_BUF_POOL_CONFG_CMD *dsp_cmd, MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 1);
	*dsp_msg = G_dsp_obj->memm_update_frm_buf_pool_config_msg;
}
EXPORT_SYMBOL(dsp_mm_update_frm_buf_pool_config);

void dsp_mm_request_fb(MEMM_REQ_FRM_BUF_CMD *dsp_cmd, MEMM_REQ_FRM_BUF_MSG *dsp_msg)
{
	dsp_cmd->callback_id = dsp_cmd->cmd_code;
	__dsp_issue_cmd_pair_msg(dsp_cmd, sizeof(*dsp_cmd), 1);
	*dsp_msg = G_dsp_obj->memm_req_frm_buf_msg;
}
EXPORT_SYMBOL(dsp_mm_request_fb);

void dsp_mm_register_fb(u16 fb_id)
{
	MEMM_REG_FRM_BUF_CMD dsp_cmd;
	dsp_cmd.cmd_code = CMD_MEMM_REG_FRM_BUF;
	dsp_cmd.frm_buf_id = fb_id;
	dsp_issue_cmd_sync_ex(&dsp_cmd, sizeof(dsp_cmd));
}
EXPORT_SYMBOL(dsp_mm_register_fb);

void dsp_mm_release_fb(u16 fb_id)
{
	MEMM_REL_FRM_BUF_CMD dsp_cmd;
	dsp_cmd.cmd_code = CMD_MEMM_REL_FRM_BUF;
	dsp_cmd.frm_buf_id = fb_id;
	dsp_issue_cmd_sync_ex(&dsp_cmd, sizeof(dsp_cmd));
}
EXPORT_SYMBOL(dsp_mm_release_fb);

static int alloc_vm_pages(dsp_vm_space_t *vms)
{
	int i;

	vms->nr_pages = 0;
	vms->pfn = NULL;

	vms->nr_pages = PAGE_ALIGN(vms->size) >> PAGE_SHIFT;
	if (vms->nr_pages == 0)
		return -EINVAL;

	vms->pfn = kzalloc(vms->nr_pages * sizeof(u32), GFP_KERNEL);
	if (vms->pfn == NULL) {
		vms->nr_pages = 0;
		return -ENOMEM;
	}

	for (i = 0; i < vms->nr_pages; i++) {
		struct page *page = alloc_page(DSP_PAGE_MASK);
		if (page == NULL)
			break;
		vms->pfn[i] = page_to_pfn(page);
	}

	if (i < vms->nr_pages) {
		for (i--; i >= 0; i--)
			__free_page(pfn_to_page(vms->pfn[i]));

		kfree(vms->pfn);
		vms->nr_pages = 0;
		vms->pfn = NULL;

		return -ENOMEM;
	}

	iav_dbg_printk("alloc_vm_pages: %d pages\n", vms->nr_pages);

	return 0;
}

static void free_vm_pages(dsp_vm_space_t *vms)
{
	int i;

	if (vms == NULL || vms->nr_pages == 0 || vms->pfn == NULL)
		return;

	iav_dbg_printk("free_vm_pages: %d pages\n", vms->nr_pages);

	for (i = 0; i < vms->nr_pages; i++)
		__free_page(pfn_to_page(vms->pfn[i]));

	kfree(vms->pfn);
	vms->nr_pages = 0;
	vms->pfn = NULL;
}

static int alloc_space_id(void)
{
	int i;

	for (i = DSP_MIN_SPACE_ID; i <= DSP_MAX_SPACE_ID; i++) {
		int mask = (1 << i);
		if ((G_dsp_obj->free_space_id_bitmap & mask) == 0) {
			G_dsp_obj->free_space_id_bitmap |= mask;
			iav_dbg_printk("alloc_space_id: %d\n", i);
			return i;
		}
	}

	iav_dbg_printk("No space id available\n");
	return -ENOMEM;
}

static void free_space_id(int space_id)
{
	int mask;

	if (space_id <= 0)
		return;

	iav_dbg_printk("free_space_id: %d\n", space_id);

	if (space_id < DSP_MIN_SPACE_ID || space_id > DSP_MAX_SPACE_ID)
		BUG();

	mask = 1 << space_id;
	if ((G_dsp_obj->free_space_id_bitmap & mask) == 0)
		BUG();

	G_dsp_obj->free_space_id_bitmap &= ~mask;
}

int dsp_alloc_vm_space(dsp_vm_space_t *vms)
{
	int rval;

	if ((rval = alloc_vm_pages(vms)) < 0)
		return rval;

	if ((vms->space_id = alloc_space_id()) < 0) {
		free_vm_pages(vms);
		return -ENOMEM;
	}

	return 0;
}

void dsp_free_vm_space(dsp_vm_space_t *vms)
{
	free_vm_pages(vms);
	free_space_id(vms->space_id);
	vms->space_id = 0;
}

static inline dsp_vm_space_t *dsp_mode_vms(void)
{
	return &G_dsp_obj->mode_vms;
}

int dsp_alloc_mode_vms(unsigned long size)
{
#ifdef CONFIG_DSP_USE_VIRTUAL_MEMORY
	if (!G_enable_vm)
		return 0;
	else {
		dsp_vm_space_t *vms = dsp_mode_vms();
		vms->size = size;
		return dsp_alloc_vm_space(vms);
	}
#else
	return 0;
#endif
}
EXPORT_SYMBOL(dsp_alloc_mode_vms);

dsp_vm_space_t *dsp_get_mode_vms(void)
{
	return dsp_mode_vms();
}
EXPORT_SYMBOL(dsp_get_mode_vms);

int dsp_get_mode_space_id(void)
{
	return dsp_get_mode_vms()->space_id ?: G_dsp_obj->dsp_buffer_space_id;
}
EXPORT_SYMBOL(dsp_get_mode_space_id);

int __dsp_map_vms(struct vm_area_struct *vma, dsp_vm_space_t *vms, unsigned long addr,
	unsigned long dsp_virt_addr, unsigned long size, pgprot_t prot)
{
	unsigned long page_index;
	unsigned long nr_pages;
	unsigned long *pfn;
	int err = 0;

	iav_dbg_printk("map_vms: addr=0x%lx, dsp_virt=0x%lx, size=0x%lx\n", addr, dsp_virt_addr, size);

	if (DSP_SPACE_ID(dsp_virt_addr) != vms->space_id) {
		BUG();
	}

	page_index = DSP_SPACE_OFFSET(dsp_virt_addr) >> PAGE_SHIFT;
	nr_pages = size >> PAGE_SHIFT;

	if (page_index + nr_pages < page_index || page_index + nr_pages >= vms->nr_pages) {
		iav_dbg_printk("map_vms: page_index=%ld, nr_pages=%ld, total_pages=%d\n",
			page_index, nr_pages, vms->nr_pages);
		return -EINVAL;
	}

	pfn = vms->pfn + page_index;
	for (; nr_pages > 0; nr_pages--) {
		err = remap_pfn_range(vma, addr, *pfn, PAGE_SIZE, prot);
		if (err < 0) {
			iav_dbg_printk("remap failed: %d\n", err);
			break;
		}
		pfn++;
		addr += PAGE_SIZE;
	}

	iav_dbg_printk("map_vms returns %d\n", err);

	return err;
}
EXPORT_SYMBOL(__dsp_map_vms);

