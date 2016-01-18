/*
 * kernel/private/drivers/eis/arch_s2_queue/eis_queue.c
 *
 * History:
 *    2013/01/05 - [Zhenwu Xue] Create
 *    2013/09/24 - [Louis Sun] Modify it to support file ops for user space
 *
 * Copyright (C) 2004-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
//static struct timeval vcap_time;

//static struct timeval      cur_time, old_time;
//static int counter = 0;




//put a new gyro data entry into queue, if it's full, report queue full and ignore the data input
static inline void gyro_data_enqueue(amba_eis_controller_t * controller, gyro_data_t *data)
{
        if (controller->ring_buffer_full) {
           // printk(KERN_DEBUG "EIS gyro queue is already full, cannot enqueue and discard \n");
            return;
         }

//     spin_lock(&controller->data_lock);
        //check whether buffer full
        if (unlikely( (((!controller->ring_buffer_read_index) && (controller->ring_buffer_write_index == controller->ring_buffer_entry_num - 1)) ||
            (controller->ring_buffer_write_index +  1 ==  controller->ring_buffer_read_index)))) {

                controller->ring_buffer_full = 1;
       }
//      spin_unlock(&controller->data_lock);

        #if 0
        if (controller->ring_buffer_full) {
            printk(KERN_DEBUG "EIS gyro queue full !!!, read_index %d, write_index %d \n", controller->ring_buffer_read_index, controller->ring_buffer_write_index );
        }
        #endif


        if (!controller->ring_buffer_full) {
            controller->gyro_ring_buffer[controller->ring_buffer_write_index] = *data;
            controller->ring_buffer_write_index++;
            if (controller->ring_buffer_write_index>= controller->ring_buffer_entry_num)
                controller->ring_buffer_write_index = 0;
       }


}


void gyro_callback(gyro_data_t *data,  void * arg)
{
#if 0
        //Gyro_callback Frequency test
        if (counter%1000 == 0) {
            do_gettimeofday(&cur_time);
            printk(KERN_DEBUG "diff is %ld uS \n",  (cur_time.tv_sec -  old_time.tv_sec)  * 1000000L  + (cur_time.tv_usec - old_time.tv_usec));
            old_time = cur_time;
        }
        counter++;
#endif

    //the returned data
    amba_eis_controller_t * controller = (amba_eis_controller_t *) arg;
    //put into queue
    gyro_data_enqueue(controller, data);
}

static irqreturn_t amba_eis_vin_isr(int irq, void *dev_data)
{
        amba_eis_controller_t * controller = (amba_eis_controller_t *) dev_data;
	//do_gettimeofday(&vcap_time);
       //printk(KERN_DEBUG "eis_queue: amba_eis_vin_isr, time %ld\n", vcap_time.tv_sec * 1000000L + vcap_time.tv_usec);
        //printk(KERN_DEBUG "EIS vin0_idsp handler \n");
        //record the write index position, note that frame_sync_index is the "last valid write index" 's next position
        controller->ring_buffer_frame_sync_index = controller->ring_buffer_write_index;
        if (controller->wait_count)  {
            complete(&controller->frame_compl);
            controller->wait_count--;
        }

	return IRQ_HANDLED;
}


int eis_start_stat(amba_eis_controller_t * controller)
{



    return 0;
}


int eis_stop_stat(amba_eis_controller_t * controller)
{



    return 0;
}


void eis_exit(amba_eis_controller_t * controller)
{
        //stop enqueue Gyro stat
       eis_stop_stat(controller);
        //unregister Gyro stat
	gyro_unregister_eis_callback();

       if (controller->gyro_ring_buffer) {
            kfree(controller->gyro_ring_buffer);
       }

	if (controller->vin_irq) {
		free_irq(controller->vin_irq, controller);
	}
}



int eis_init(amba_eis_controller_t * controller)
{
        int ret;

        //clear data in controller
        memset(controller, 0, sizeof(*controller));
        //allcoate ring buffer
        controller->gyro_ring_buffer = kzalloc(MAX_ENTRY_IN_RING_BUFFER * sizeof(gyro_data_t), GFP_KERNEL);
        if (!controller->gyro_ring_buffer) {
            printk(KERN_DEBUG "Eis driver unable to alloc gyro ring buffer \n");
            return -1;
        }
        controller->ring_buffer_entry_num = MAX_ENTRY_IN_RING_BUFFER;

        //init completion
        init_completion(&controller->frame_compl);

        //register VIN IRQ for Gyro data output
       controller->vin_irq = EIS_VIN_IRQ;
	ret = request_irq(controller->vin_irq, amba_eis_vin_isr,
		IRQF_TRIGGER_RISING | IRQF_SHARED, "EIS", controller);
	if (ret) {
		printk("%s: eis_queue: Fail to request eis vin irq!\n", __func__);
		controller->vin_irq = 0;
		return -1;
	}

        //reigster gyro interrupt callback
      gyro_register_eis_callback(gyro_callback, controller);


    return 0;
}




int amba_eis_start_stat(amba_eis_context_t * context)
{



    return 0;
}


int amba_eis_stop_stat(amba_eis_context_t * context)
{



    return 0;
}

static inline int get_eis_stat_count_till_frame_sync(amba_eis_controller_t * controller)
{
    int count;
    int frame_sync_index = controller->ring_buffer_frame_sync_index;

    if(frame_sync_index >= controller->ring_buffer_read_index) {
        count =  frame_sync_index -  controller->ring_buffer_read_index;
    } else {
        count =  (frame_sync_index - 0) + (controller->ring_buffer_entry_num - controller->ring_buffer_read_index);
    }

    if (count == 0 ) {
        printk(KERN_DEBUG "ZERO:   frame_sync_index = %d,  read_index=%d \n", frame_sync_index, controller->ring_buffer_read_index);
   }

    return count;
}


static inline int copy_stat_records(amba_eis_controller_t * controller,  amba_eis_stat_t * eis_stat , int record_num, int discard_flag)
{
    int start_read_index;
    int frame_sync_index = controller->ring_buffer_frame_sync_index;


    if (unlikely(!record_num)) {
            return -1;
     }

    //record_num must be positive integer and within scope
    eis_stat->gyro_data_count = record_num;
    eis_stat->discard_flag = discard_flag;

    if (!discard_flag) {
        start_read_index = controller->ring_buffer_read_index;
    } else {
        //discard means records are too many, only fit the record_num to output
        start_read_index = frame_sync_index - record_num;
        if (start_read_index < 0)
            start_read_index+= controller->ring_buffer_entry_num;
    }

    if (start_read_index < frame_sync_index) {
        //check
        if (frame_sync_index - start_read_index != record_num) {
            printk(KERN_DEBUG "eis copy LOGIC error ! \n");
            return -1;
        }
        memcpy(eis_stat->gyro_data, controller->gyro_ring_buffer + start_read_index, record_num * sizeof(gyro_data_t));
   } else if (start_read_index > frame_sync_index) {
        //check
        if ((controller->ring_buffer_entry_num - start_read_index) + frame_sync_index != record_num) {
            printk(KERN_DEBUG "eis copy LOGIC error AGAIN ! \n");
            return -1;
        }
        memcpy(eis_stat->gyro_data, controller->gyro_ring_buffer + start_read_index,  (controller->ring_buffer_entry_num - start_read_index) * sizeof(gyro_data_t));
        memcpy(eis_stat->gyro_data + (controller->ring_buffer_entry_num - start_read_index),
            controller->gyro_ring_buffer ,  frame_sync_index * sizeof(gyro_data_t));
   } else {
         printk(KERN_DEBUG "Eis copy LOGIC strange ! \n");
         return -1;
   }

    //advance read pointer, with wrap around handling

//    spin_lock(&controller->data_lock);
    if (start_read_index + record_num < controller->ring_buffer_entry_num)
        controller->ring_buffer_read_index = start_read_index + record_num ;
    else
        controller->ring_buffer_read_index = start_read_index + record_num - controller->ring_buffer_entry_num;
//    spin_unlock(&controller->data_lock);
//    printk(KERN_DEBUG "read_index is %d , discard %d \n", controller->ring_buffer_read_index, discard_flag);

    //clear full flag
    controller->ring_buffer_full = 0;
    return 0;
}


//this function returns by the time of a FRAME START
int amba_eis_get_stat(amba_eis_context_t * context,  amba_eis_stat_t * eis_stat)
{
    int record_num;
    int discard_flag = 0;       //when there are too many to read, and beyond the output buffer, then discard the very old data
    amba_eis_controller_t * controller = context->controller;

    //if eis has not start (state not OK) , return error

    //if vin is not running, then no way to do frame sync,  return error

    controller->wait_count++;
    //else, should be OK to wait for frame sync.
    if(wait_for_completion_interruptible(&controller->frame_compl)) {
            printk(KERN_DEBUG  "Eis unable to wait for completion! \n");
            return -EIO;
    }

    //decide the data to fill into return value.


    //if the data is more than the eis_stat can hold, then return max entry number (CUT from old to most recent)
    //otherwise, just return the actual number of records (from oldest to most recent)

    record_num = get_eis_stat_count_till_frame_sync(controller);
    if (record_num > GYRO_DATA_ENTRY_MAX_NUM) {
            record_num = GYRO_DATA_ENTRY_MAX_NUM;
            discard_flag = 1;
    }
    else if (record_num == 0) {
        printk(KERN_DEBUG "Error: Eis get null gyro data on frame sync !  \n");
        memset(eis_stat, 0, sizeof(*eis_stat));
        return 0;
    }

    return copy_stat_records(controller, eis_stat, record_num, discard_flag);

}

int amba_eis_get_info(amba_eis_context_t * context,  amba_eis_info_t * eis_info)
{


    return 0;
}

