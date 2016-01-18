#include <fcntl.h>

#include "stdio.h"
#include "am_new.h"
#include "rec_pretreat_if.h"
#include "vout_control_if.h"
typedef IRecPreTreat RP;
/*#define CHECK_ERR_RETURN(err, log) \
    if(err != ME_OK){ \
        printf(#log); \
        return err; \
    }
*/
static RP* g_RP;
static IVoutControl* VC;

struct OptionStruct
{
    AM_INT v_x;
    AM_INT v_y;
    AM_INT v_w;
    AM_INT v_h;
    AM_INT o_x;
    AM_INT o_y;
    AM_INT o_w;
    AM_INT o_h;
};
#define VOUT_NUM 2
static OptionStruct G_option[VOUT_NUM];

enum V_INDEX
{
    HDMI = 0,
    LCD,
};

AM_INT ParseOption(int argc, char **argv)
{
    int i = 0, k = 0;
    for(i = 1; i < argc; i++)
    {
        //i = k;
        if(!strcmp("-hdmi",argv[i]))
        {
            i++;
            if(!strcmp("-video", argv[i]))
            {
                    sscanf(argv[i+1], "%d", &G_option[HDMI].v_w);
                    sscanf(argv[i+2], "%d", &G_option[HDMI].v_h);
                    sscanf(argv[i+3], "%d", &G_option[HDMI].v_x);
                    sscanf(argv[i+4], "%d", &G_option[HDMI].v_y);
                    i += 4;
            }
            if(!strcmp("-osd", argv[i])){
                    sscanf(argv[i], "%d", &G_option[HDMI].o_w);
                    sscanf(argv[i], "%d", &G_option[HDMI].o_h);
                    sscanf(argv[i], "%d", &G_option[HDMI].o_x);
                    sscanf(argv[i], "%d", &G_option[HDMI].o_y);
                    i +=4;
            }
        }else if(!strcmp("-lcd",argv[i])){
            i++;
            if(!strcmp("-video", argv[i]))
            {
                    sscanf(argv[i+1], "%d", &G_option[LCD].v_w);
                    sscanf(argv[i+2], "%d", &G_option[LCD].v_h);
                    sscanf(argv[i+3], "%d", &G_option[LCD].v_x);
                    sscanf(argv[i+4], "%d", &G_option[LCD].v_y);
                    i += 4;
            }
            if(!strcmp("-osd", argv[i])){
                    sscanf(argv[i], "%d", &G_option[LCD].v_w);
                    sscanf(argv[i], "%d", &G_option[LCD].v_h);
                    sscanf(argv[i], "%d", &G_option[LCD].v_x);
                    sscanf(argv[i], "%d", &G_option[LCD].v_y);
                    i +=4;
            }
        }
    }
    printf("show option:\n");
    printf("HDMI: size:%d,  %d, pos:%d, %d\n", G_option[HDMI].v_w, G_option[HDMI].v_h, G_option[HDMI].v_x, G_option[HDMI].v_y);
    printf("LCD: size:%d,  %d, pos:%d, %d\n", G_option[LCD].v_w, G_option[LCD].v_h, G_option[LCD].v_x, G_option[LCD].v_y);

    return 0;
}

static AM_INT mIavFd;

static CParam gPar(2);
static CParam gPar1(2);
static CParam gPar2(2);


inline static int test_en()
{
    printf("test disable and enable\n");
    printf("===========disable hdmi osd...============\n");
    if(VC->DisableVoutStream(IVoutControl::HDMI_OSD) != ME_OK)
    {
        printf("1 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========disable hdmi video...============\n");
    if(VC->DisableVoutStream(IVoutControl::HDMI_VIDEO) != ME_OK)
    {
        printf("2 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========enable hdmi video...============\n");

    if(VC->EnableVoutStream(IVoutControl::HDMI_VIDEO) != ME_OK)
    {
        printf("3 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========enable hdmi osd...============\n");
    if(VC->EnableVoutStream(IVoutControl::HDMI_OSD) != ME_OK)
    {
        printf("4 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========disable lcd osd...============\n");
    if(VC->DisableVoutStream(IVoutControl::LCD_OSD) != ME_OK)
    {
        printf("1 Failed!\n");
       // return -1;
    }
    sleep(2);
    printf("===========disable lcd video...============\n");
    if(VC->DisableVoutStream(IVoutControl::LCD_VIDEO) != ME_OK)
    {
        printf("2 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========enable lcd video...============\n");

    if(VC->EnableVoutStream(IVoutControl::LCD_VIDEO) != ME_OK)
    {
        printf("3 Failed!\n");
        //return -1;
    }
    sleep(2);
    printf("===========enable lcd osd...============\n");
    if(VC->EnableVoutStream(IVoutControl::LCD_OSD) != ME_OK)
    {
        printf("4 Failed!\n");
        //return -1;
    }
    sleep(1);
    return 0;
}


inline static int test_osd_size()
{
    /*TEST osd in hdmi*/
    printf("\ntest change osd size in hdmi and lcd\n");

    if(VC->GetVoutConfig(IVoutControl::SIZE_HDMI_OSD, gPar) != ME_OK)
    {
        printf("1\n");
       // return -1;
    }
    printf("GetVoutConfig  SIZE_HDMI_OSD: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 500;
    gPar1[1] = 500;
    printf("===========change osd size in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI_OSD, gPar1) != ME_OK)
    {
        printf("2 Failed!\n");
        //return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    if(gPar1[0] == 0 || gPar1[1] ==0)
    {
        gPar1[0] = 700;
        gPar1[1] = 400;
    }
    printf("===========revert the size of osd in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI_OSD, gPar1) != ME_OK)
    {
        printf("3 Failed!\n");
       // return -1;
    }
    sleep(2);

    /*---------------------------------
    //TEST osd in lcd  cannot chang on 10-17
    if(VC->GetVoutConfig(IVoutControl::SIZE_LCD_OSD, gPar) != ME_OK)
    {
        printf("4\n");
       //return -1;
    }
    printf("GetVoutConfig  SIZE_LCD_OSD: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 200;
    gPar1[1] = 200;
    printf("===========change osd size in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_LCD_OSD, gPar1) != ME_OK)
    {
        printf("5 Failed!\n");
        //return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    if(gPar1[0] == 0 || gPar1[1] ==0)
    {
        gPar1[0] = 700;
        gPar1[1] = 400;
    }
    printf("===========revert the size of osd in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_LCD_OSD, gPar1) != ME_OK)
    {
        printf("6 Failed!\n");
        //return -1;
    }
    sleep(2);
    -----------------*/
    return 0;
}

inline static int test_osd_position()
{
    /*TEST osd in hdmi*/
    printf("\ntest change osd position in hdmi and lcd\n");

    if(VC->GetVoutConfig(IVoutControl::POSITION_HDMI_OSD, gPar) != ME_OK)
    {
        printf("1\n");
        //return -1;
    }
    printf("GetVoutConfig  POSITION_HDMI_OSD: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 200;
    gPar1[1] = 200;
    printf("===========change osd position in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_HDMI_OSD, gPar1) != ME_OK)
    {
        printf("2 Failed!\n");
        //return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the position of osd in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_HDMI_OSD, gPar1) != ME_OK)
    {
        printf("3 Failed!\n");
       // return -1;
    }
    sleep(2);
    /*TEST osd in lcd*/
    if(VC->GetVoutConfig(IVoutControl::POSITION_LCD_OSD, gPar) != ME_OK)
    {
        printf("4\n");
        //return -1;
    }
    printf("GetVoutConfig  POSITION_LCD_OSD: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 200;
    gPar1[1] = 200;
    printf("===========change osd position in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_LCD_OSD, gPar1) != ME_OK)
    {
        printf("5 Failed!\n");
        //return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the position of osd in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_LCD_OSD, gPar1) != ME_OK)
    {
        printf("6 Failed!\n");
       // return -1;
    }
    sleep(2);
    return 0;

}

inline static int test_video_size()
{
    /*TEST osd in hdmi*/
    printf("\ntest change video size in hdmi and lcd\n");
/*
    if(VC->GetVoutConfig(IVoutControl::SIZE_HDMI_VIDEO, gPar) != ME_OK)
    {
        printf("1\n");
        //return -1;
    }
    printf("GetVoutConfig  SIZE_HDMI_VIDEO: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 720;
    gPar1[1] = 480;
    printf("===========change video size in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI_VIDEO, gPar1) != ME_OK)
    {
        printf("2 Failed!\n");
       // return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the size of video in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI_VIDEO, gPar1) != ME_OK)
    {
        printf("3 Failed!\n");
       // return -1;
    }
    sleep(2);

*/
    /*TEST video in lcd*/
    if(VC->GetVoutConfig(IVoutControl::SIZE_LCD_VIDEO, gPar) != ME_OK)
    {
        printf("4\n");
        //return -1;
    }
    printf("GetVoutConfig  SIZE_LCD_VIDEO: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 480;
    gPar1[1] = 580;
    printf("===========change osd video in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_LCD_VIDEO, gPar1) != ME_OK)
    {
        printf("5 Failed!\n");
       // return -1;
    }
    sleep(2);
    /*
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the size of video in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_LCD_VIDEO, gPar1) != ME_OK)
    {
        printf("6 Failed!\n");
        //return -1;
    }*/
    sleep(2);
    return 0;
}

inline static int test_video_position()
{
    /*TEST osd in hdmi*/
    printf("\ntest change video position in hdmi and lcd\n");
/*
    if(VC->GetVoutConfig(IVoutControl::POSITION_HDMI_VIDEO, gPar) != ME_OK)
    {
        printf("1\n");
       // return -1;
    }
    printf("GetVoutConfig  POSITION_HDMI_VIDEO: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 500;
    gPar1[1] = 500;
    printf("===========change  video position in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_HDMI_VIDEO, gPar1) != ME_OK)
    {
        printf("2 Failed!\n");
        //return -1;
    }
    sleep(2);
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the position of video in hdmi...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_HDMI_VIDEO, gPar1) != ME_OK)
    {
        printf("3 Failed!\n");
       // return -1;
    }
    sleep(2);
  */
    /*TEST video in lcd*/
    if(VC->GetVoutConfig(IVoutControl::POSITION_LCD_VIDEO, gPar) != ME_OK)
    {
        printf("4\n");
        //return -1;
    }
    printf("GetVoutConfig  POSITION_HDMI_OSD: %d %d\n", gPar[0], gPar[1]);

    gPar1[0] = 0;
    gPar1[1] = 55;
    printf("===========change video position in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_LCD_VIDEO, gPar1) != ME_OK)
    {
        printf("5 Failed!\n");
        //return -1;
    }
    sleep(2);
    /*
    gPar1[0] = gPar[0];
    gPar1[1] = gPar[1];
    printf("===========revert the position of video in lcd...============\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_HDMI_OSD, gPar1) != ME_OK)
    {
        printf("6 Failed!\n");
       // return -1;
    }
    */
    sleep(2);
    return 0;


}

static inline int test_video_flip()
{


    return 0;
}

static void sigstop(int a)
{
    if(mIavFd >= 0)
    {
        close(mIavFd);
    }

    //AM_DELETE(g_RP);
    AM_DELETE(VC);
}


int main(int argc, char** argv)
{
    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);


    //CHECK_ERR_RETURN(ME_ERROR, "TEST ERROR MACRO\n");
    if (ParseOption(argc, argv)) {
        return 1;
    }

    AM_ERR err;

    if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0)
    {
        AM_ERROR("open /dev/iav");
        return ME_OS_ERROR;
    }

    g_RP = AM_CreateRecPreTreat(mIavFd);
    if(g_RP == NULL)
    {
        printf("Create RecPreTreat Failed!\n");
        return -1;
    }
    if((VC = AM_CreateVoutControl(mIavFd)) == NULL)
    {
        AM_WARNING("VoutControl Create Failed!\n");
        err =VC->CheckVout();
        if(err != ME_OK)
        {
            VC->TryEnableVout();
        }
    }

    AM_WARNING("VoutControl Create:%p\n", VC);
    //sleep(3);
     AM_WARNING("SetVout");
             CParam param(4);
            param[0] = 240;
            param[1] = 640;
            param[2] = 60;  // kuang's up/down
            param[3] = 30; //left

        if(g_RP->SetVout(IRecPreTreat::VOUT_LCD, param) != ME_OK)
        {
            LOGW("g_RP SetVout Failed!");
            //return UNKNOWN_ERROR;
        }
        sleep(1);
             AM_WARNING("AdjuctVout");

        if(g_RP->AdjuctVout() != ME_OK)
        {
            LOGW("g_RP AdjuctVout Failed!");
            //return UNKNOWN_ERROR;
        }
    sleep(5);
/*
    VC =(IVoutControl*) g_RP->GetVO();
    if(VC ==NULL)
    {
        printf("Create IVoutControl Failed!\n");
        return -1;
    }
*/
/*
    if(g_RP->EnterPreview(320, 240) != ME_OK)
    {
        printf("Enter Privew Failed!\n");
        return -1;
    }
    sleep(3);

    test_en();

    //test_osd_size();

    test_video_size();
    test_video_position();
    /*
    sleep(5);
    printf("\n\nBegin test set vout!\n");
    CParam param(2);
    param[0] = 500;//x
    param[1] = 500;//y
    //param[2] = 320;//w
    //param[3] = 240;//h
    //sleep(1);
    printf("Set Position!\n");
    VC->GetVoutConfig(IVoutControl::POSITION_HDMI_OSD, gPar);
    VC->GetVoutConfig(IVoutControl::CONFIG_NUM, gPar1);

    printf("Get: Position %d, %d..........Dev width:%d, height %d\n", gPar[0], gPar[1], gPar1[0], gPar1[1]);
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI_OSD, param) != ME_OK)
    {
        printf("SetVout Failed!\n");
        //return UNKNOWN_ERROR;
    }
    sleep(10);


    */

    /*
    /*param[0] = G_option[HDMI].v_w;//x
    param[1] = G_option[HDMI].v_h;//y

    printf("==>Set HDMI Video Size!\n");
    VC->GetVoutConfig(IVoutControl::SIZE_LCD_VIDEO, gPar);
    printf("Get: Size %d, %d!!\n", gPar[0], gPar[1]);
    if(VC->SetVoutConfig(IVoutControl::SIZE_LCD_VIDEO, param) != ME_OK)
    {
        printf("SetVout Failed!\n");
        //return UNKNOWN_ERROR;
    }
    sleep(7);

    /*
    sleep(1);
    param[0] = 500;//x
    param[1] = 500;//y
    printf("Set HDMI Video Offset!\n");
    if(VC->SetVoutConfig(IVoutControl::SIZE_HDMI, param) != ME_OK)
    {
        printf("SetVout Failed!\n");
        //return UNKNOWN_ERROR;
    }
    /*
    sleep(1);
    printf("Begin test set vout!\n");
    if(VC->SetVoutConfig(IVoutControl::POSITION_LCD, param) != ME_OK)
    {
        printf("SetVout Failed!\n");
        //return UNKNOWN_ERROR;
    }*/
    //
    sleep(3);
    printf("End test set vout!\n\n\n");
    if(mIavFd >= 0)
    {
        close(mIavFd);
    }

    //AM_DELETE(g_RP);
    AM_DELETE(VC);

    return 0;
}
