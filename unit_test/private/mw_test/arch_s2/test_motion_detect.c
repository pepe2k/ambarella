#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sched.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include <signal.h>
#include "mw_struct.h"
#include "mw_api.h"

#define MAX_ALARM_SIZE      (128)
int running = 1;
int display_event_flag = 0;

typedef enum {
    S_L = 50,
    S_M = 20,
    S_H = 5,
}sensitivity_type;

char alarm_msg[MW_MAX_ROI_NUM][MAX_ALARM_SIZE]= {
    "          ",
    "          ",
    "          ",
    "          ",};

// The whole frame is divided into 8*12 tiles.
//default roi location are as follows.
//  ________________________________
// |               |                |
// |   ROI#0 4*6   |    ROI#1 4*6   |
// |_______________|________________|
// |               |                |
// |   ROI#2 4*6   |    ROI#3 4*6   |
// |_______________|________________|

static mw_roi_info  roi_info[MW_MAX_ROI_NUM] = {
	{0, 0, 6, 4, 50, S_H, 1},  //ROI0 x = 0, y = 0, w = 6, h = 4
	{6, 0, 6, 4, 50, S_H, 1},  //ROI1 x = 0, y = 4, w = 6, h = 4
	{0, 4, 6, 4, 50, S_M, 1},  //ROI2 x = 6, y = 0, w = 6, h = 4
	{6, 4, 6, 4, 50, S_L, 1}	//ROI3 x = 6, y = 4, w = 6, h = 4
};

static int my_md_alarm(const int* p_motion_event)
{
    int i;
    for (i = 0; i < MW_MAX_ROI_NUM; i++) {
        if (p_motion_event[i] == EVENT_MOTION_START)
            snprintf(&alarm_msg[i][0], MAX_ALARM_SIZE, "   START  ");
        else if (p_motion_event[i] == EVENT_MOTION_END)
            snprintf(&alarm_msg[i][0], MAX_ALARM_SIZE, "    END   ");
    }
    fprintf(stderr,"\r\t|%s|%s|%s|%s|",
                    alarm_msg[0],alarm_msg[1],alarm_msg[2],alarm_msg[3]);
    fflush(stderr);
    return 0;
}

static void sigstop()
{
    printf("\n\nStop Motion Detection.\n\n");
    mw_md_thread_stop();
    running = 0;
    exit(1);
}

void md3a_show_menu()
{
    printf("\n _______________________________\n\t\t\t\t|\n");
    printf("| Main Menu\t\t\t|\n|\t\t\t\t|\n");
    printf("| 1 - View ROI configuration\t|\n");
    printf("| 2 - Set ROI Configuration\t|\n");
    printf("| 3 - Display Detection Result\t|\n");
    printf("|_______________________________|\n");
}

void md3a_show_roi_setup_menu(int roi_idx, mw_roi_info *info)
{
    printf("\n _______________________________\n|\t\t\t\t|\n");
    printf("| ROI#%d SETUP\t\t\t|\n|\t\t\t\t|\n", roi_idx);
    printf("| 1 - Valid [%d]\t\t\t|\n", info->valid);
    printf("| 2 - Threshold [%d]\t\t|\n", info->threshold);
    printf("| 3 - Sensitivity [%s]\t|\n",
            (info->sensitivity >= S_L ? "low ":
                (info->sensitivity <= S_H ? "high" : "medium")));
    printf("| 4 - Back to Main Menu\t\t|\n");
    printf("|_______________________________|\n");
}

static int md3a_show_roi_info()
{
    int i;

    printf("\t ______________________________\n");
    printf("\t|               |              |\n");
    printf("\t|     ROI #1    |     ROI #2   |\n");
    printf("\t|_______________|______________|\n");
    printf("\t|               |              |\n");
    printf("\t|     ROI #3    |     ROI #4   |\n");
    printf("\t|_______________|______________|\n\n");

    for ( i = 0; i < MW_MAX_ROI_NUM; i++){
        mw_roi_info info;
        mw_md_get_roi_info(i, &info);
        printf("ROI#%d: x %d, y %d, width %d, height %d, threshold %d, sensitivity %s, valid %d\n",
                i+1, info.x, info.y, info.width, info.height, info.threshold,
                (info.sensitivity >= S_L ? "low":(info.sensitivity <=S_H ? "high":"medium")), info.valid);
    }
    return 0;
}

static int md3a_set_roi_info(int roi_idx)
{
    char input[32];
    int value, opt;
    int flag = 1;
    mw_roi_info info;
    if (roi_idx < 0 || roi_idx >= MW_MAX_ROI_NUM) {
        printf("ROI index out of range (1 ~ %d)./n", MW_MAX_ROI_NUM);
        return -1;
    }
    mw_md_get_roi_info(roi_idx, &info);

    while (flag) {
        md3a_show_roi_setup_menu(roi_idx, &info);
        printf("\nYour choice: ");
        scanf("%s", input);
	 opt = atoi(input);

        switch (opt){
            case 1:
                do {
                printf("Enter valid (0:disable, 1: enable): ");
		  fflush(stdin);
                scanf("%s", input);
		  value = atoi(input);
                } while (value != 0 && value != 1);
                info.valid = value;
                break;
            case 2:
                do {
                printf("Enter new threshold (1 ~ %d): ", MW_MAX_MD_THRESHOLD);
		  fflush(stdin);
                scanf("%s", input);
		  value = atoi(input);
                } while (value < 1 || value > MW_MAX_MD_THRESHOLD);
                info.threshold = value;
                break;
            case 3:
                do {
                printf("Enter new sensitivity (1:Low, 2:Medium, 3:High): ");
		  fflush(stdin);
                scanf("%s", input);
		  value = atoi(input);
                } while (value < 1 || value > 3);
                info.sensitivity = (value == 1 ? S_L : ((value == 2) ? S_M : S_H));
                break;
            case 4:
                flag = 0;
                break;
            default:
                printf("Illegal Option index.\n");
                break;
        }
    }

    if (mw_md_set_roi_info(roi_idx, &info) != 0) {
        return -1;
    }
    return 0;
}



int main(int argc, char**argv)
{
    int roi_idx, opt;
    int i = 4;
    char input[32];

    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    mw_md_callback_setup(my_md_alarm);
    mw_md_thread_start();

    for (i = 0; i < MW_MAX_ROI_NUM; i++)
        mw_md_set_roi_info(i, &roi_info[i]);

    while(running) {
        if (!display_event_flag) {
            md3a_show_menu();
            printf("\nYour choice: ");
	     fflush(stdin);
            scanf("%s", input);
	     opt = atoi(input);

            switch (opt) {
                case 1:
                    if (md3a_show_roi_info() < 0) {
                        printf("\nGet ROI info failed!\n");
                    }
                    break;
                case 2:
                    do {
                        printf("Type the ROI index (1 ~ %d): ", MW_MAX_ROI_NUM);
			   fflush(stdin);
                        scanf("%s", input);
			   roi_idx = atoi(input);

                    } while (roi_idx < 1 || roi_idx > MW_MAX_ROI_NUM);
                    if (md3a_set_roi_info(roi_idx - 1) < 0) {
                        printf("\nSet ROI info failed!\n");
                    }
                    break;
                case 3:
                    display_event_flag = 1;
                    printf("\n\t __________ __________ __________ __________\n");
                    printf("\t|          |          |          |          |\n");
                    printf("\t|  ROI #1  |  ROI #2  |  ROI #3  |  ROI #4  |\n");
                    printf("\t|          |          |          |          |\n");
                    mw_md_print_event(display_event_flag);
                    break;
                default:
                    printf("\nWrong option number.\n");
                    break;
            }
        }
        usleep(300*1000);
    }
    return 0;
}
