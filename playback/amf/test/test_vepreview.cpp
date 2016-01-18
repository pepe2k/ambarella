// OpenGL ES 1.0 code

#define LOG_NDEBUG 0
#define LOG_TAG "VETestPreview"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "am_types.h"
#include "osal.h"
#include "am_if.h"
#include "am_pbif.h"
#include "am_ve_if.h"
#include "am_mw.h"
#include "am_util.h"
#include <video_effecter_if.h>

#include <binder/MemoryHeapBase.h>
#include <media/AudioTrack.h>
#include "MediaPlayerService.h"
#include "AMPlayer.h"
/*
extern "C" {
#include "libavcodec/avcodec.h"
}*/

//==========================================
//                      OpenGL part
//==========================================


#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES/glext.h>

#include <basetypes.h>
#include <ambas_stream_texture.h>
#include "ambas_event.h"
#include <ui/FramebufferNativeWindow.h>
#include <ui/EGLUtils.h>

//#define print_framerate
#ifdef print_framerate
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
static AM_U32 time_diff=0;
static AM_U32 time_gpu=0;
AM_INT frames=0;
static struct timeval pre_time = {0,0};
static struct timeval curr_time = {0,0};
static struct timeval pre_timegpu = {0,0};
static struct timeval curr_timegpu = {0,0};
#endif

using namespace android;

static int	 Stream_Texture_Device_created = 0;
static int	 Stream_Texture_Buffer_rendered = 0;

GLuint texture = 0;
GLuint texture1 = 0;

#define NUM_STREAM_DEVICES	2
#define BUFFERS_PER_DEVICE	5
#define SIZE_PER_BUFFER		0x00400000

#define FB_WIDTH	1280
#define FB_HEIGHT	800//720
#define FB_PITCH	1280

static PFNGLTEXBINDSTREAMIMGPROC  myglTexBindStreamIMG = NULL;
static PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC  myglGetTexStreamDeviceAttributeivIMG = NULL;
static PFNGLGETTEXSTREAMDEVICENAMEIMGPROC  myglGetTexStreamDeviceNameIMG = NULL;
static int  stream_texture_fd;

static STREAM_TEXTURE_DEVINFO  dev_info[NUM_STREAM_DEVICES];
static STREAM_TEXTURE_BUFFER  texture_buffers[NUM_STREAM_DEVICES][BUFFERS_PER_DEVICE];

STREAM_TEXTURE_FB2INFO fb2_info;
static const char *fb2_mem = NULL;
static int event_fd = -1;


static EGLNativeWindowType window;
EGLSurface eglSurface;
EGLDisplay eglDisplay;
EGLContext eglContext;

extern void SetGPUFuncs(
        //int (*InitStreamTexture)(), int (*InitGLES)(), 
        int (*RenderTextureBuffer)(bool [],int []),
        int (*CreateStreamTextureDevice)(bool [],int [], int [], int [], unsigned int [][5])
        //int (*DestroyStreamTextureDevice)()
        );

int VE_Create_Stream_Texture_Device(bool used[],int width[], int height[], int pitch[], unsigned int InputBufferPool[][5]);

int VE_Render_Buffer(bool used[], int inputbufferID[]);


GLfloat x=1.0;//if use pitch creactly in Create_Stream_Texture_Device,this should be 1.
GLfloat y=1;//1080.0/1088.0;//480.0/1280.0;//pic_height may be not equal to buffer_height, then texture need cut to satisfy the polygon

static GLfloat vertices[] =//front
{
    -1,    -1,    1,
    1,    -1,    1,
    1,    1,    1,
    -1,    1,    1,
};

static GLfloat texcoord[] =
{
    0,   y,   0,
    x,  y, 0,
    x,  0,  0,
    0,  0,  0
};

static GLfloat vertices1[] =//right
{
    1,    -1,    1,
    1,    -1,    -1,
    1,    1,    -1,
    1,    1,    1,
};

static GLfloat vertices2[] = //back
{
    1,    -1,    -1,
    -1,    -1,    -1,
    -1,    1,    -1,
    1,    1,    -1,
};

static GLfloat vertices3[] =//left
{
    -1,    -1,    -1,
    -1,    -1,    1,
    -1,    1,    1,
    -1,    1,    -1,
};

static GLfloat vertices4[] =//top
{
    -1,    1,    1,
    1,    1,    1,
    1,    1,    -1,
    -1,    1,    -1,
};

static GLfloat vertices5[] =//bottom
{
    -1,    -1,    -1,
    1,    -1,    -1,
    1,    -1,    1,
    -1,    -1,    1,
};

static GLfloat verticesbackground[] =//background
{
    -6,    -6,    -6,
    6,    -6,    -6,
    6,    6,    -6,
    -6,    6,    -6,
};

static void gluLookAt(float eyeX, float eyeY, float eyeZ,
	float centerX, float centerY, float centerZ, float upX, float upY,
	float upZ)
{
    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:

    float fx = centerX - eyeX;
    float fy = centerY - eyeY;
    float fz = centerZ - eyeZ;

    // Normalize f
    float rlf = 1.0f / sqrtf(fx*fx + fy*fy + fz*fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // Normalize up
    float rlup = 1.0f / sqrtf(upX*upX + upY*upY + upZ*upZ);
    upX *= rlup;
    upY *= rlup;
    upZ *= rlup;

    // compute s = f x up (x means "cross product")

    float sx = fy * upZ - fz * upY;
    float sy = fz * upX - fx * upZ;
    float sz = fx * upY - fy * upX;

    // compute u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    float m[16] ;
    m[0] = sx;
    m[1] = ux;
    m[2] = -fx;
    m[3] = 0.0f;

    m[4] = sy;
    m[5] = uy;
    m[6] = -fy;
    m[7] = 0.0f;

    m[8] = sz;
    m[9] = uz;
    m[10] = -fz;
    m[11] = 0.0f;

    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;

    glMultMatrixf(m);
    glTranslatef(-eyeX, -eyeY, -eyeZ);
}

#if 0
int gotdata=0;

static void Fb2_Pan_Display(int signo)
{
    struct amb_event events[256];
    struct amb_event event;
    unsigned int i, sno, pan;
    int ret;

    printf("\nFb2_Pan_Display~~~~~~%d\n", gotdata);
    if(event_fd < 0 || !fb2_mem) {
        return;
    }

    ret = ::read(event_fd, events, sizeof(events));
    if(ret <= 0) {
        printf("nothing\n");
        return;
    }

    pan = 0;
    for(i = 0, sno = 0; i < ret / sizeof(struct amb_event); i++ ) {
        if(events[i].type == AMB_EV_FB2_PAN_DISPLAY) {
            if(events[i].sno > sno) {
                sno = events[i].sno;
                event = events[i];
                pan = 1;
                gotdata++;
                break;
            }
        }
    }

    if (pan) {
        unsigned int yoffset;

        memcpy(&yoffset, event.data, 4);
        //start: fb2_mem + fb2_info.width * 2 * yoffset
        //len: fb2_info.width * fb2_info.height * 2
        printf("yoffset: %u\n", yoffset);
    }
//    myglTexBindStreamIMG(0, 0);
//    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
//    eglSwapBuffers(dpy, surface);
}
#endif

int VE_init_gl_surface(void)
{
    EGLint numConfigs = 1;
    EGLConfig myConfig;
    EGLint attrib[] =
    {
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };
    EGLint w, h;
    printf("in VE_init_gl_surface\n");
    if ( (eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY )
    {
        printf("eglGetDisplay failed\n");
        return -1;
    }
    printf("eglGetDisplay done\n");
    if ( eglInitialize(eglDisplay, NULL, NULL) != EGL_TRUE )
    {
        printf("eglInitialize failed\n");
        return -1;
    }
    printf("eglInitialize done\n");
    EGLNativeWindowType window = android_createDisplaySurface();
    EGLUtils::selectConfigForNativeWindow(eglDisplay, attrib, window, &myConfig);
    printf("android_createDisplaySurface done\n");
    eglSurface = eglCreateWindowSurface(eglDisplay, myConfig, window, NULL);
    printf("eglCreateWindowSurface done\n");
    if ( (eglContext = eglCreateContext(eglDisplay, myConfig, NULL, NULL)) == EGL_NO_CONTEXT )
    {
        printf("eglCreateContext failed, VE_Create_FB2 in front of VE_init_gl_surface?\n");
        //return -1;
    }
    printf("eglCreateContext done\n");
    if ( eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) != EGL_TRUE )
    {
        printf("eglMakeCurrent failed\n");
        //return -1;
    }
    printf("eglMakeCurrent done\n");
    eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &w);
    eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &h);
    printf("w=%d, h=%d\n", w, h);
    printf("VE_init_gl_surface done\n");
    return 0;
}
#if 0
void VE_Init_GLES()
{
    EGLint configAttribs[] = {
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };

    EGLint majorVersion;
    EGLint minorVersion;
    EGLContext context;
    EGLConfig config;
    EGLint w, h;

    window = android_createDisplaySurface();

    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(dpy, &majorVersion, &minorVersion);

    status_t err = EGLUtils::selectConfigForNativeWindow(dpy, configAttribs, window, &config);
    if (err) {
        fprintf(stderr, "couldn't find an EGLConfig matching the screen format\n");
        return;
    }

    surface = eglCreateWindowSurface(dpy, config, window, NULL);
    context = eglCreateContext(dpy, config, NULL, NULL);
    eglMakeCurrent(dpy, surface, surface, context);
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);

    printf("w=%d, h=%d\n", w, h);
}
#endif

void VE_Init_Scene(int width, int height)
{

    //glDisable(GL_DITHER);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    float ratio = width / height;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-ratio*1, ratio*1, -1, 1, 1, 10);

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    gluLookAt(
            0, 0, 3.5,  // eye
            0, 0, 0,  // center
            0, 1, 0); // up
}

static AM_ERR VE_Init_Stream_Texture(void)
{
    const GLubyte *pszGLExtensions;

    pszGLExtensions = glGetString(GL_EXTENSIONS);

    printf("get GL_EXTENSIONS = %s.\n", pszGLExtensions);
    /* GL_IMG_texture_stream */
    if (strstr((char *)pszGLExtensions, "GL_IMG_texture_stream")){
        myglTexBindStreamIMG = (PFNGLTEXBINDSTREAMIMGPROC)eglGetProcAddress("glTexBindStreamIMG");
        myglGetTexStreamDeviceAttributeivIMG =
            (PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC)eglGetProcAddress("glGetTexStreamDeviceAttributeivIMG");
        myglGetTexStreamDeviceNameIMG =
            (PFNGLGETTEXSTREAMDEVICENAMEIMGPROC)eglGetProcAddress("glGetTexStreamDeviceNameIMG");

        if(!myglTexBindStreamIMG || !myglGetTexStreamDeviceAttributeivIMG || !myglGetTexStreamDeviceNameIMG){
            printf("eglGetProcAddress() failed for texture stream extension\n");
            return ME_ERROR;
        }
    }
    else{
        printf("No driver support for GL_IMG_texture_stream extension\n");
        return ME_ERROR;
    }
    printf("Get extention done!\n");
#if 1 //this part just print some debug info of stream texture:
    const GLubyte *pszStreamDeviceName;
    int NumDevices=0;
    static int numBuffers, bufferWidth, bufferHeight, bufferFormat;

    glGetIntegerv(GL_TEXTURE_NUM_STREAM_DEVICES_IMG, &NumDevices);
    printf("NumDevices = %d.\n",NumDevices);

    pszStreamDeviceName = myglGetTexStreamDeviceNameIMG(0);
    myglGetTexStreamDeviceAttributeivIMG(0, GL_TEXTURE_STREAM_DEVICE_NUM_BUFFERS_IMG, &numBuffers);
    myglGetTexStreamDeviceAttributeivIMG(0, GL_TEXTURE_STREAM_DEVICE_WIDTH_IMG, &bufferWidth);
    myglGetTexStreamDeviceAttributeivIMG(0, GL_TEXTURE_STREAM_DEVICE_HEIGHT_IMG, &bufferHeight);
    myglGetTexStreamDeviceAttributeivIMG(0, GL_TEXTURE_STREAM_DEVICE_FORMAT_IMG, &bufferFormat);

    printf("\nStream Device %s: numBuffers = %d, width = %d, height = %d, format = %x\n",
         pszStreamDeviceName, numBuffers, bufferWidth, bufferHeight, bufferFormat);
#endif

    return ME_OK;
}

AM_INT VEGLESInit(AM_INT width, AM_INT height)
{
    VE_Init_Scene(width, height);
    printf("VE: init_scene done!!\n");

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor (0.2, 0.2, 0.2, 0);
//    glShadeModel (GL_SMOOTH);

    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);

    glColor4f(0.0, 0.0, 0.0, 0.0);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

    glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenTextures(1, &texture);
    glGenTextures(1, &texture1);

    glEnable(GL_TEXTURE_STREAM_IMG);
    printf("VE: Init Done!\n");
    return 0;
}

AM_ERR VE_Create_FB2()
{
    //create fb2
    if (::ioctl(stream_texture_fd, STREAM_TEXTURE_GET_OUTPUT_DEVICE, &fb2_info) == BCE_OK) {
        printf("FB2 has been created: width = %lu, height = %lu, format = %d!\n",
            fb2_info.width, fb2_info.height, fb2_info.format);
    } else {
        fb2_info.width = 480;//FB_WIDTH;
        fb2_info.height = 800;//FB_HEIGHT;
        fb2_info.format = PVRSRV_PIXEL_FORMAT_RGB565;
        fb2_info.buffer[0] = (void *)0x01500000; // last 4MB of PPM area
        fb2_info.buffer[1] = (void *)(0x01500000 + FB_WIDTH * FB_HEIGHT * 2 * 1);
        fb2_info.buffer[2] = (void *)(0x01500000 + FB_WIDTH * FB_HEIGHT * 2 * 2);
        fb2_info.buffer[3] = (void *)(0x01500000 + FB_WIDTH * FB_HEIGHT * 2 * 3);
        fb2_info.buffer[4] = (void *)(0x01500000 + FB_WIDTH * FB_HEIGHT * 2 * 4);
        fb2_info.n_buffer = 5;
        fb2_info.size = FB_WIDTH * FB_HEIGHT * 2 * 5;
        if (::ioctl(stream_texture_fd, STREAM_TEXTURE_CREATE_OUTPUT_DEVICE, &fb2_info) == BCE_OK) {
            printf("FB2 Created!\n");
        } else {
            printf("FB2 Create Failed!\n");
            return ME_ERROR;
        }
    }

    if (ioctl(stream_texture_fd, STREAM_TEXTURE_MAP_OUTPUT_BUFFER, NULL) != BCE_OK) {
        printf("STREAM_TEXTURE_MAP_OUTPUT_BUFFER error!\n");
        return ME_ERROR;
    }

    fb2_mem = (const char *)mmap(NULL, fb2_info.size, PROT_READ | PROT_WRITE, MAP_SHARED,
        stream_texture_fd, NULL);
    if (!fb2_mem) {
        printf("Fail to map fb2!\n");
        return ME_ERROR;
    } else {
        printf("Fb2: 0x%p\n", fb2_mem);
    }

    return ME_OK;
}

AM_ERR VE_Open_Stream_Texture()
{
    stream_texture_fd = ::open("/dev/stream_texture", O_RDWR | O_NDELAY);
    if (stream_texture_fd < 0) {
        printf("VE: Unable to open device node stream_texture!\n");
        return ME_ERROR;
    };

    for (int i = 0; i< 8; i++)
        ::ioctl(stream_texture_fd, STREAM_TEXTURE_DESTROY_INPUT_DEVICE, i);

    return ME_OK;
}
//int GPUCreateStreamTextureDevice(int width, int height, int pitch, void* ptr)
int VE_Create_Stream_Texture_Device(bool used[],int width[], int height[], int pitch[], unsigned int InputBufferPool[][5])
{
    int i, j;

    if(VE_Open_Stream_Texture()<0){
        printf("VE: Open stream_texture(bc) error!\n");
        return -1;
    }

    printf("VE: InputBufferPool: %p %p %p %p %p; %p %p %p %p %p.\n",
        (void*)InputBufferPool[0][0],(void*)InputBufferPool[0][1], (void*)InputBufferPool[0][2],(void*)InputBufferPool[0][3],(void*)InputBufferPool[0][4],
        (void*)InputBufferPool[1][0],(void*)InputBufferPool[1][1], (void*)InputBufferPool[1][2],(void*)InputBufferPool[1][3],(void*)InputBufferPool[1][4]);

    //create stream device
    for (i = 0; i < NUM_STREAM_DEVICES; i++) {
        ::memset(&dev_info[i], 0, sizeof(dev_info[i]));
        dev_info[i].ulNumBuffers			= BUFFERS_PER_DEVICE;
        dev_info[i].sBufferInfo.pixelformat	= PVRSRV_PIXEL_FORMAT_NV12;
       // dev_info[i].sBufferInfo.pixelformat	= PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV;
        dev_info[i].sBufferInfo.ui32BufferCount = BUFFERS_PER_DEVICE;
        dev_info[i].sBufferInfo.ui32Width		= pitch[i];
        dev_info[i].sBufferInfo.ui32Height	= 800;
        dev_info[i].sBufferInfo.ui32ByteStride	= pitch[i];
        sprintf(dev_info[i].sBufferInfo.szDeviceName, "amba texture %d", i);
        dev_info[i].sBufferInfo.ui32Flags  	= PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE | PVRSRV_BC_FLAGS_YUVCSC_BT601;
        ::memset(texture_buffers[i], 0, sizeof(texture_buffers[i]));
        for (j = 0; j < BUFFERS_PER_DEVICE; j++) {
            texture_buffers[i][j].ulBufferID = j;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
            texture_buffers[i][j].sSysAddr.uiAddr = used[i]?InputBufferPool[i][j]:InputBufferPool[0][j]; //((j==4)?0x197F0240:InputBufferPool[j]);
            texture_buffers[i][j].sPageAlignSysAddr.uiAddr = (texture_buffers[i][j].sSysAddr.uiAddr & 0xFFFFF000);
#endif
            texture_buffers[i][j].ulSize = dev_info[i].sBufferInfo.ui32ByteStride * dev_info[i].sBufferInfo.ui32Height;
            texture_buffers[i][j].ulSize += (texture_buffers[i][j].ulSize >> 1);
            //texture_buffers[i][j].ulSize = dev_info[i].sBufferInfo.ui32ByteStride * dev_info[i].sBufferInfo.ui32Height*3/2;
        }
        dev_info[i].psSystemBuffer = texture_buffers[i];

        if (::ioctl(stream_texture_fd, STREAM_TEXTURE_CREATE_INPUT_DEVICE, &dev_info[i]) == BCE_OK) {

            printf("Device[%d] id is %d!\n",i, (int)dev_info[i].sBufferInfo.ui32BufferDeviceID);
        } else {
            Stream_Texture_Device_created = 0;
            ::close(stream_texture_fd);
            printf("Unable to create stream device!\n");
            return -1;
        }
    }
    printf("VE: VE_Create_Stream_Texture_Device done.\n");

//VE_Create_FB2 must in front of VE_init_gl_surface
    if(VE_Create_FB2()==ME_ERROR){
        printf("VE: VE_Create_FB2 failed.\n");
        return -1;
    }
    printf("VE: VE_Create_FB2 done.\n");

    if(VE_init_gl_surface()<0){
        printf("VE: Open VE_init_gl_surface() error!\n");
        return -1;
    }
    printf("VE: VE_init_gl_surface done.\n");

    if(VEGLESInit(FB_WIDTH, FB_HEIGHT)<0){
        printf("======VEGLESInit(%d,%d) Error!======\n",FB_WIDTH, FB_WIDTH);
        return -1;
    }
    printf("VE: VEGLESInit done.\n");

    if(VE_Init_Stream_Texture()<0){
        printf("======VE_Init_Stream_Texture() Error!======\n");
        return -1;
    }
    printf("VE: VE_Init_Stream_Texture done.\n");

    Stream_Texture_Device_created = 1;
    return 0;
}

void VERenderStep(bool used[], int inputbufferID[])
{
#if 0
{
    static int i, j = 0;

	if (!app_inited) {
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);

	//update buffer
	if (++j > (BUFFERS_PER_DEVICE - 1)) {
		j = 0;
	}

	for (i = 0; i < NUM_STREAM_DEVICES; i++) {
		//update stream buffer
		texture_buffers[i][j].ulBufferDeviceID = dev_info[i].sBufferInfo.ui32BufferDeviceID;
		if (ioctl(stream_texture_fd, STREAM_TEXTURE_SET_INPUT_BUFFER, &texture_buffers[i][j]) == BCE_OK) {
			//LOGI("Buffer id is %d!", (int)texture_buffers[i][j].ulBufferID);
		} else {
			LOGE("Unable to set texture buffer: %d %d!", (int)texture_buffers[i][j].ulBufferDeviceID, (int)texture_buffers[i][j].ulBufferID);
		}
		myglTexBindStreamIMG(dev_info[i].sBufferInfo.ui32BufferDeviceID, j);
	}
	glEnable(GL_TEXTURE_STREAM_IMG);

	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
	//Fb2_Pan_Display();
	sleep(1);
	return;
}
#endif
#if 0
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	//update buffer
	if (++j > (BUFFERS_PER_DEVICE - 1)) {
		j = 0;
	}
	for (i = 0; i < NUM_STREAM_DEVICES; i++) {
		//update stream buffer
		texture_buffers[i][j].ulBufferDeviceID = dev_info[i].sBufferInfo.ui32BufferDeviceID;
		if (ioctl(stream_fd, STREAM_TEXTURE_SET_INPUT_BUFFER, &texture_buffers[i][j]) == BCE_OK) {
			//LOGI("Buffer id is %d!", (int)texture_buffers[i][j].ulBufferID);
		} else {
			LOGE("Unable to set texture buffer: %d %d!", (int)texture_buffers[i][j].ulBufferDeviceID, (int)texture_buffers[i][j].ulBufferID);
		}
		myglTexBindStreamIMG(dev_info[i].sBufferInfo.ui32BufferDeviceID, j);
	}
	glEnable(GL_TEXTURE_STREAM_IMG);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
#endif

    glPushMatrix();
    glLoadIdentity();
    glVertexPointer   (3, GL_FLOAT, 0, verticesbackground);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    myglTexBindStreamIMG(dev_info[0].sBufferInfo.ui32BufferDeviceID, inputbufferID[0]);
    glBindTexture(GL_TEXTURE_STREAM_IMG, texture);
    glFlush();
    //glFinish();
    glPopMatrix();

    static GLfloat eye_angle = 0;
    static GLfloat eye_step = 0.5;
//    if(eye_angle>45) eye_step*=-1;
//    if(eye_angle<-45) eye_step*=-1;
    eye_angle += eye_step;
    glRotatef(eye_step, 0, 0, 1);
    static GLfloat eye_angle2 = 0;
    static GLfloat eye_step2 = 0.8;
//    if(eye_angle2>45) eye_step2*=-1;
//    if(eye_angle2<-45) eye_step2*=-1;
    eye_angle2 += eye_step2;
    glRotatef(eye_step2, 1, 0, 0);
    static GLfloat eye_angle3 = 0;
    static GLfloat eye_step3 = 0.3;
//    if(eye_angle3>45) eye_step3*=-1;
//    if(eye_angle3<-45) eye_step3*=-1;
    eye_angle3 += eye_step3;
    glRotatef(eye_step3, 0, 1, 0);

    glVertexPointer   (3, GL_FLOAT, 0, vertices);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    glVertexPointer   (3, GL_FLOAT, 0, vertices1);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    glVertexPointer   (3, GL_FLOAT, 0, vertices2);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    glVertexPointer   (3, GL_FLOAT, 0, vertices3);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    glVertexPointer   (3, GL_FLOAT, 0, vertices4);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    glVertexPointer   (3, GL_FLOAT, 0, vertices5);
    glTexCoordPointer (3, GL_FLOAT, 0, texcoord);
    glDrawArrays (GL_TRIANGLE_FAN, 0, 4);
    myglTexBindStreamIMG(dev_info[1].sBufferInfo.ui32BufferDeviceID, used[1]? inputbufferID[1]:inputbufferID[0]);
    glBindTexture(GL_TEXTURE_STREAM_IMG, texture1);

    glFlush();
    glFinish();
    usleep(10000); //glFinish() do nothing, so sleep here to ensure the FB has been finished painting
    eglSwapBuffers(eglDisplay, eglSurface);
    AM_DBG("VE: One Frame Render Done!\n");

}

int VE_Render_Buffer(bool used[], int inputbufferID[])
{
    AM_INFO("VE_Render_Buffer: ID[0]=%d, ID[1]=%d\n",inputbufferID[0], inputbufferID[1]);
#ifdef print_framerate
static u32 d_timediff = 0;
    if(time_diff == 0)
        gettimeofday(&pre_time, NULL);

//fps control
    if(d_timediff < 1000000/30){
    //    usleep(1000000/30-d_timediff);
    }
    gettimeofday(&pre_timegpu, NULL);
#endif

    myglTexBindStreamIMG = (PFNGLTEXBINDSTREAMIMGPROC)eglGetProcAddress("glTexBindStreamIMG");
    if(myglTexBindStreamIMG==NULL){
        printf("VE: (myglTexBindStreamIMG!=NULL!\n");
    }else{
//        myglTexBindStreamIMG(dev_info[0].sBufferInfo.ui32BufferDeviceID, inputbufferID);
//        glGenTextures(1, &texture);
//        myglTexBindStreamIMG(dev_info[0].sBufferInfo.ui32BufferDeviceID, inputbufferID);
    }
    VERenderStep(used, inputbufferID);
    Stream_Texture_Buffer_rendered = 1;

#ifdef print_framerate
    gettimeofday(&curr_time, NULL);
    time_diff = (curr_time.tv_sec - pre_time.tv_sec) * 1000000 + curr_time.tv_usec - pre_time.tv_usec;
    frames++;
    d_timediff = (curr_time.tv_sec - pre_timegpu.tv_sec) * 1000000 + curr_time.tv_usec - pre_timegpu.tv_usec;
    time_gpu += d_timediff;

    if(time_diff>=10000000){
        printf("VE: ====framerate = %f. (%d frames/%u us). GPU time = %u us.=======\n", (frames*1.0)/(time_diff*1.0/1000000),frames,time_diff,time_gpu);
        time_diff = 0;
        time_gpu = 0;
        frames = 0;
    }
#endif

//    printf("VE: VE_Render_Buffer done!\n");
    return 0;
}

AM_INT VE_Destroy_Stream_Texture_Device()
{
    if (::ioctl(stream_texture_fd, STREAM_TEXTURE_DESTROY_INPUT_DEVICE, &dev_info[0].ulDeviceID) == BCE_OK) {
        Stream_Texture_Device_created = 0;
        printf("VE: VE_Destroy_Stream_Texture_Device!\n");
    } else {
        printf("VE: Unable to VE_Destroy_Stream_Texture_Device!\n");
        return -1;
    }
    return 0;
}

//==========================================
//                      playback part
//==========================================

//AMPlayer::AudioSink *audiosink;

IVEControl *G_pVEControl;
CEvent *G_pTimerEvent;
CThread *G_pTimerThread;

void ProcessAMFMsg(void *context, AM_MSG& msg)
{
    printf("AMF msg: %d\n", msg.code);

    if (msg.code == IMediaControl::MSG_PLAYER_EOS)
        printf("==== Playback end ====\n");
}

AM_ERR TimerThread(void *context)
{
    while (1) {
        if (G_pTimerEvent->Wait(1000) == ME_OK)
            break;
        //
    }
    return ME_OK;
}

int GetVEInfo(IVEControl::VE_INFO &info)
{
	AM_ERR err;
	if ((err = G_pVEControl->GetVEInfo(info)) != ME_OK) {
		printf("GetVEInfo failed\n");
		return -1;
	}
		printf("GetVEInfo done\n");
	return 0;
}

void DoPause()
{
	AM_ERR err;
	IVEControl::VE_INFO info;

	if (GetVEInfo(info) < 0)
		return;
//todo
	return;
}

void DoGoto(AM_U64 ms)
{
//todo
}

void DoPrintState()
{
#ifdef AM_DEBUG
    G_pVEControl->PrintState();
#endif
}

void RunMainLoop()
{
    char buffer_old[128] = {0};
    char buffer[128];
    IVEControl::VE_INFO info;
    static int flag_stdin = 0;
    printf("======RunMainLoop!======\n");

    //read keyboard input will not block the while
    flag_stdin = fcntl(STDIN_FILENO,F_GETFL);
    if(fcntl(STDIN_FILENO,F_SETFL,fcntl(STDIN_FILENO,F_GETFL) | O_NONBLOCK) == -1)
        printf("stdin_fileno set error");

    while (1) {
        G_pVEControl->GetVEInfo(info);
        if(info.state == IVEControl::STATE_ERROR){
            printf("======Play Error!======\n");
            return;
        }
        if(info.state == IVEControl::STATE_COMPLETED){
            printf("======Play Completed!======\n");
            return;
        }
	if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0)
		continue;

	if (buffer[0] == '\n')
		buffer[0] = buffer_old[0];
		switch (buffer[0]) {
			case 'q':	// exit
				printf("Quit\n");

				if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
					printf("stdin_fileno set error");
				return;

			case ' ':	// pause
				buffer_old[0] = buffer[0];
				printf("DoPause\n");
				DoPause();
				break;

			case 'p':   //print state, debug only
				printf("DoPrintState.\n");
				DoPrintState();
				break;

			case 'g':
			{
				AM_UINT seconds;
				AM_UINT ms;
				seconds = atoi(&buffer[1]);
				char *pms = strchr(&buffer[1], '.');
				if (pms == NULL) {
					ms = 0;
				}
				else {
					pms++;
					int len = strlen(pms);
					if (len != 3) {
						printf("Error: must specify 3 digits for ms\n");
						break;
					}
					ms = atoi(pms);
				}
				printf("seek to: %d.%03d\n", seconds, ms);
				DoGoto(seconds * 1000 + ms);	// seek
			}
			break;
		default:
			break;
	}
    }
    if(fcntl(STDIN_FILENO,F_SETFL,flag_stdin) == -1)
        printf("stdin_fileno set error");
}

AM_INT ParseOption(int argc, char **argv)
{
    if(argc<2){
        AM_INFO("test_vepreview stream\n");
        AM_INFO("test_vepreview stream1 stream2\n");
        return 1;
    }
    return 0;
}

static void sigstop(int signo)
{
      printf("received signal %d\n", signo);
      exit(1);
}

int main(int argc, char **argv)
{
    AM_ERR err;
    AM_UINT Source1, Source2;
    char configfilename[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};

    signal(SIGSEGV, sigstop);


    if (ME_OK != AMF_Init()) {
        AM_ERROR("AMF_Init Error.\n");
        return -2;
    }

    //read global cfg from pb.config, optional
    snprintf(configfilename, sizeof(configfilename), "%s/pb.config", AM_GetPath(AM_PathConfig));
    AM_LoadGlobalCfg(configfilename);
    snprintf(configfilename, sizeof(configfilename), "%s/log.config", AM_GetPath(AM_PathConfig));
    AM_LoadLogConfigFile(configfilename);
    AM_OpenGlobalFiles();
    //AM_GlobalFilesSaveBegin(argv[1]);

    if (ParseOption(argc, argv)) {
        return 1;
    }

    if (AMF_Init() != ME_OK) {
        AM_ERROR("AMF_Init() failed\n");
        return -1;
    }

    if ((G_pTimerEvent = CEvent::Create()) == NULL) {
        AM_ERROR("Cannot create event\n");
        return -1;
    }

    G_pVEControl = CreateActiveVEControl(NULL);

    if ((G_pVEControl->SetAppMsgCallback(ProcessAMFMsg, NULL)) != ME_OK) {
        AM_ERROR("SetAppMsgCallback failed\n");
        return -1;
    }
    if ((G_pTimerThread = CThread::Create("timer", TimerThread, NULL)) == NULL) {
        AM_ERROR("Create timer thread failed\n");
        return -1;
    }

    printf("======VEEngine Created!======\n");
    SetGPUFuncs( &VE_Render_Buffer, &VE_Create_Stream_Texture_Device);
    printf("======SetGPUFuncs done!======\n");

    err = G_pVEControl->AddSource(argv[1],IVEControl::Type_Video,Source1);
    if( err == ME_OK){
        AM_INFO("PrepareFile url=%s done.\n", argv[1]);
    }else{
        AM_ERROR("PrepareFile url=%s failed.\n", argv[1]);
        return -1;
    }

    if(argc>2 && (NULL!=argv[2])){
        AM_INFO("PrepareFile url=%s.\n", argv[2]);
        err = G_pVEControl->AddSource(argv[2],IVEControl::Type_Video,Source2);
        if( err == ME_OK){
            AM_INFO("PrepareFile url=%s done.\n", argv[2]);
        }else{
            AM_ERROR("PrepareFile url=%s failed.\n", argv[2]);
        return -1;
        }
    }

    AM_INFO("StartPreview ...\n");
    err = G_pVEControl->StartPreview(1);
    if( err == ME_OK){
        AM_INFO("StartPreview done.\n");
    }else{
        AM_INFO("StartPreview failed.\n");
        return -1;
    }

/*
    if(VE_Open_Stream_Texture()<0){
        printf("VE: Open stream_texture(bc) error!\n");
        return -1;
    }
    unsigned int pool[5]={NULL,NULL,NULL,NULL,NULL};
    if(VE_Create_Stream_Texture_Device(PIC_WIDTH,PIC_HEIGHT,PIC_PITCH,pool)<0){
        printf("VE: Open VE_Create_Stream_Texture_Device() error!\n");
        return -1;
    }
    if(VE_Create_FB2()==ME_ERROR){
        printf("VE: VE_Create_FB2 failed.\n");
        return -1;
    }
    if(VE_init_gl_surface()<0){
        printf("VE: VE_init_gl_surface() error!\n");
        return -1;
    }
    if(VEGLESInit(PIC_WIDTH, PIC_HEIGHT)<0){
        printf("======VEGLESInit(%d,%d) Error!======\n",PIC_WIDTH, PIC_HEIGHT);
        return -1;
    }
*/

    RunMainLoop();

    G_pVEControl->Stop();
    G_pTimerEvent->Signal();
    G_pTimerThread->Delete();
    G_pVEControl->Delete();
    AM_INFO("G_pVEControl->Delete() done.\n");
    G_pTimerEvent->Delete();
    AM_INFO("G_pTimerEvent->Delete() done.\n");

    //todo - cause crash
    AMF_Terminate();
    AM_INFO("AMF_Terminate() done.\n");

    return ME_OK;
}

