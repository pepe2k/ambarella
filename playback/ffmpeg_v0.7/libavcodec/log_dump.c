#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "log_dump.h"

int log_cur_frame=0;
int log_start_frame=1;
int log_end_frame=0;

int log_start_mb_x=0;
int log_start_mb_y=0;
int log_end_mb_x=4;
int log_end_mb_y=4;

int log_mask[16]={1,1,1,1, 1,1,0,0, 0,0,0,0, 0,0,0,0};
static int log_mask_rev[16]={1,1,1,1, 1,1,1,0, 0,0,0,0, 0,0,0,0};
int log_config_start_frame=0;
static int log_config=0;

int dump_config_extedge=0;

char log_mbtypestr[tot_mbtype_cnt][20]=
{
{"INTRA"},
{"DIRECT"},
{"SKIP16x16"},
{"SKIPGMC16x16"},
{"GMC16x16"},
{"FIELD16x8"},
{"FIELD16x8BW"},
{"FIELD16x8BI"},
{"FRAME16x16"},
{"FRAME16x16BW"},
{"FRAME16x16BI"},
{"FRAME8x8"},
};

#define max_entry 40
static FILE* pfile[5*max_entry+1]={0};

#undef printf
#undef fprintf

#define _name_without_config_
#define _name_with_surfix_

static int _log_debug_trap()
{
    static int cnt=0;
    cnt++;
    return 0;
}

void set_decoding_config(int config,int start_frame)
{
    int i=0;
    log_config=config;
    log_config_start_frame=start_frame;
    for(i=0;i<16;i++)
    {
        log_mask_rev[i]=config&(1<<i);
    }
    printf("log_mask[decoding_config_idct_deq]=%d.\n",log_mask_rev[decoding_config_idct_deq]);
    printf("log_mask[decoding_config_add_idct]=%d.\n",log_mask_rev[decoding_config_add_idct]);
    printf("log_mask[decoding_config_mc]=%d.\n",log_mask_rev[decoding_config_mc]);
    printf("log_mask[decoding_config_intrapred]=%d.\n",log_mask_rev[decoding_config_intrapred]);
    printf("log_mask[decoding_config_deblock]=%d.\n",log_mask_rev[decoding_config_deblock]);
    printf("log_mask[decoding_config_pre_interpret_gmc]=%d.\n",log_mask_rev[decoding_config_pre_interpret_gmc]);
    printf("log_mask[decoding_config_use_dsp_permutated]=%d.\n",log_mask_rev[decoding_config_use_dsp_permutated]);
}

void apply_decoding_config()
{
    printf("apply_decoding_config.\n");
    int i=0;
    for(i=0;i<16;i++)
    {
        log_mask[i]=log_mask_rev[i];
    }
}

void set_dump_config(int start, int end, int start_x, int end_x, int start_y,int end_y, int dump_ext)
{
    log_start_frame=start;
    log_end_frame=end;
    log_start_mb_x=start_x;
    log_start_mb_y=start_y;
    log_end_mb_x=end_x;
    log_end_mb_y=end_y;
    
    dump_config_extedge=dump_ext;
}

void get_dump_config(int *start, int *end)
{
    if(start)
        *start = log_start_frame;
    if(end)
        *end = log_end_frame;
}

int _log_debug_trap();

//with separate frame index, by 3*max_entry
int log_openfile_p(int index,char* str,int fcnt)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    if(fcnt<log_start_frame || fcnt>log_end_frame)
        return 1;
    index+=(fcnt%5)*max_entry;
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+30);
    if(!p)
        return -2;
    
#ifdef _name_with_surfix_
    #ifdef _name_without_config_
        sprintf(p,"%s_%d.dat",str,fcnt);
    #else
        sprintf(p,"%s_%d_%d.dat",str,fcnt,log_config);
    #endif
#else
    #ifdef _name_without_config_
        sprintf(p,"%s_%d",str,fcnt);
    #else
        sprintf(p,"%s_%d_%d",str,fcnt,log_config);
    #endif
#endif
    
    pfile[index]=fopen(p,"wb");
    free(p);
    return 0;
}

int log_openfile(int index,char* str)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    if(log_cur_frame<log_start_frame || log_cur_frame>log_end_frame)
        return 1;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+30);
    if(!p)
        return -2;
#ifdef _name_with_surfix_
    #ifdef _name_without_config_
        sprintf(p,"%s_%d.dat",str,log_cur_frame);
    #else
        sprintf(p,"%s_%d_%d.dat",str,log_cur_frame,log_config);
    #endif
#else
    #ifdef _name_without_config_
        sprintf(p,"%s_%d",str,log_cur_frame);
    #else
        sprintf(p,"%s_%d_%d",str,log_cur_frame,log_config);
    #endif
#endif    
    pfile[index]=fopen(p,"wb");
    free(p);
    return 0;
}

int log_openfile_f(int index,char* str)
{
//    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
        
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }
/*
    p=malloc(strlen(str)+20);
    if(!p)
        return -2;
    
    #ifdef _name_without_config_
        sprintf(p,"%s_%d",str,log_cur_frame);
    #else
        sprintf(p,"%s_%d_%d",str,log_cur_frame,log_config);
    #endif
*/    
    pfile[index]=fopen(str,"wb");
    //free(p);
    return 0;
}

int log_openfile_f_p(int index,char* str,int fcnt)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    index+=(fcnt%5)*max_entry;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+20);
    if(!p)
        return -2;
    
    #ifdef _name_without_config_
        sprintf(p,"%s_%d",str,fcnt);
    #else
        sprintf(p,"%s_%d_%d",str,fcnt,log_config);
    #endif
    
    pfile[index]=fopen(p,"wb");
    free(p);
    return 0;
}

int log_openfile_with_num(int index,char* str,int num)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    if(log_cur_frame<log_start_frame || log_cur_frame>log_end_frame)
        return 1;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+30);
    if(!p)
        return -2;

    #ifdef _name_without_config_
        sprintf(p,"%s_%d_%d",str,log_cur_frame,num);
    #else
        sprintf(p,"%s_%d_%d_%d",str,log_cur_frame,num,log_config);
    #endif
    
    pfile[index]=fopen(p,"wb");
    free(p);
    return 0;
}

int log_closefile(int index)
{
    if(index>max_entry || index<0 ||!pfile[index] )
        return -1;
    _log_debug_trap();
    fclose(pfile[index]);
    pfile[index]=NULL;
    return 0;
}

int log_closefile_p(int index,int fcnt)
{
    if(index>max_entry || index<0  )
        return -1;
    index+=(fcnt%5)*max_entry;
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }
    return 0;
}

int log_dump_with_audo_cnt(char* filename, char* data,int len)
{
    static int cnt = 0;

    char* p=NULL;
    unsigned int size = 0;

    size = strlen(filename)+30;

    p=malloc(size);
    if(!p)
        return -2;
    memset(p, 0, size);

    snprintf(p,size -1,"%s_%d",filename,cnt++);
    FILE* file =fopen(p,"wb");
    free(p);
    if (file) {
        fwrite(data,1,len,file);
        fclose(file);
    }
    return 0;
}

int log_dump_with_audo_cnt_2(char* filename, char* data,int len)
{
    static int cnt = 0;

    char* p=NULL;
    unsigned int size = 0;

    size = strlen(filename)+30;

    p=malloc(size);
    if(!p)
        return -2;
    memset(p, 0, size);

    snprintf(p,size -1,"%s_%d",filename,cnt++);
    FILE* file =fopen(p,"wb");
    free(p);
    if (file) {
        fwrite(data,1,len,file);
        fclose(file);
    }
    return 0;
}

int log_dump(int index,char* data,int len)
{
    if(index>max_entry || index<0 ||!pfile[index] )
        return -1;
    
    if(log_cur_frame<log_start_frame)
        return 1;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return 2;
    }   
    
    fwrite(data,1,len,pfile[index]);
    return 0;
}

int log_dump_f(int index,char* data,int len)
{
    if(index>max_entry || index<0 ||!pfile[index] )
        return -1;
        
    fwrite(data,1,len,pfile[index]);
    return 0;
}

int log_dump_p(int index,char* data,int len,int fcnt)
{
    if(index>max_entry || index<0 )
        return -1;
    index+=(fcnt%5)*max_entry;
    if(fcnt<log_start_frame ||!pfile[index] )
        return 1;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return 2;
    }   
    
    fwrite(data,1,len,pfile[index]);
    return 0;
}

int log_openfile_text(int index,char* str)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    if(log_cur_frame<log_start_frame || log_cur_frame>log_end_frame)
        return 1;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+20);
    if(!p)
        return -2;
    sprintf(p,"%s_%d",str,log_cur_frame);
    pfile[index]=fopen(p,"wt");
    free(p);
    return 0;
}

int log_openfile_text_p(int index,char* str,int fcnt)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    index+=(fcnt%5)*max_entry;
    if(fcnt<log_start_frame || fcnt>log_end_frame)
        return 1;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+20);
    if(!p)
        return -2;
    sprintf(p,"%s_%d",str,fcnt);
    pfile[index]=fopen(p,"wt");
    free(p);
    return 0;
}

int log_openfile_text_f(int index,char* str)
{
    char* p=NULL;
    if(index>max_entry || index<0)
        return -1;
    
    if(pfile[index])
    {
        _log_debug_trap();
        fclose(pfile[index]);
        pfile[index]=NULL;
    }

    p=malloc(strlen(str)+20);
    if(!p)
        return -2;
    sprintf(p,"%s_%d",str,log_cur_frame);
    pfile[index]=fopen(p,"wt");
    free(p);
    return 0;
}

int log_text(int index,char* str)
{
    if(index>max_entry || index<0 ||!pfile[index] || !str)
        return -1;
    
    if(log_cur_frame<log_start_frame)
        return 1;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return 2;
    }   
    
    fprintf(pfile[index],"%s\n",str);
    return 0;
}

int log_text_f(int index,char* str)
{
    if(index>max_entry || index<0 ||!pfile[index] || !str)
        return -1;

    fprintf(pfile[index],"%s\n",str);
    return 0;
}

int log_text_p(int index,char* str,int fcnt)
{
    if(index>max_entry || index<0 || !str)
        return -1;
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame || !pfile[index])
        return 1;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return 2;
    }   
    
    fprintf(pfile[index],"%s\n",str);
    return 0;
}

int log_idct_txt(int index,void* pdata,int size)
{
    int i=0,j=0;
    short *p=(short*)pdata;
    
    if(index>max_entry || index<0 ||!pfile[index] ||!pdata )
        return -1;
    
    fprintf(pfile[index],"Y:\n");
    for(i=0;i<size;i++)
    {
        for(j=0;j<size;j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p++;
        }
        fprintf(pfile[index]," \n");
    }
    return 0;
}

int log_sub_idct_txt(int index,void* pdata,int stride)
{
    int i=0,j=0;
    short *p=(short*)pdata;
    
    if(index>max_entry || index<0 ||!pfile[index] ||!pdata )
        return -1;
    
    for(i=0;i<4;i++)
    {
        for(j=0;j<4;j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p++;
        }
        fprintf(pfile[index]," \n");
        p+=stride-4;
    }
    return 0;
}

int log_idct_txt_uv(int index,void* pdata,int size)
{
    int i=0,j=0;
    short *p=(short*)pdata;
    
    if(index>max_entry || index<0 ||!pfile[index] ||!pdata )
        return -1;
    
    for(i=0;i<size;i++)
    {
        for(j=0;j<(size*2);j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p++;
        }
        fprintf(pfile[index]," \n");
    }
    return 0;
}

int log_idct_txt_uv_separated(int index,void* pdata)
{
    int i=0,j=0;
    short *p=(short*)pdata;
    
    if(index>max_entry || index<0 ||!pfile[index] ||!pdata )
        return -1;
    //u
    fprintf(pfile[index],"U:\n");
    for(i=0;i<8;i++)
    {
        for(j=0;j<8;j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p+=2;
        }
        fprintf(pfile[index]," \n");
    }
    //v
    fprintf(pfile[index],"V:\n");
    p=(short*)pdata;
    p++;
    for(i=0;i<8;i++)
    {
        for(j=0;j<8;j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p+=2;
        }
        fprintf(pfile[index]," \n");
    }
    return 0;
}

int log_dump_rect(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || width<1 ||height<1 ||stride<1 ||!data)
        return -1;
    
    if(log_cur_frame<log_start_frame)
        return 1;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return 2;
    }   
    int i=0;
    for(i=0;i<height;i++,data+=stride)
    {
        fwrite(data,1,width,pfile[index]);
    }
    return 0;
}

int log_reference_txt(int index,unsigned char* pdata,int w,int h)
{
    int i=0,j=0;
    unsigned char *p=pdata;
    
    if(index>max_entry || index<0 ||!pfile[index] ||!pdata )
        return -1;
    
    for(i=0;i<h;i++)
    {
        for(j=0;j<w;j++)
        {
            fprintf(pfile[index]," %d  ",*p);
            p++;
        }
        fprintf(pfile[index]," \n");
    }
    return 0;
}

void log_text_rect_char(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%5d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_p(int index,char* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame ||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%5d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_hex(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%02.2x ",*data++);
            fprintf(pfile[index],"%2.2x ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_hex_f(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%02.2x ",*data++);
            fprintf(pfile[index],"%2.2x ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_hex_p(int index,char* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%02.2x ",*data++);
            fprintf(pfile[index],"%2.2x ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_chroma(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%5d  ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_chroma_p(int index,char* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame)
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%5d  ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_hex_chroma(int index,char* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%02.2x ",*data++);
            fprintf(pfile[index],"%2.2x ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_char_hex_chroma_p(int index,char* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;

    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%02.2x ",*data++);
            fprintf(pfile[index],"%2.2x ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short(int index,short* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_p(int index,short* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame ||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_hex(int index,short* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%06.4hx ",*data++);
            fprintf(pfile[index],"%6.4hx ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_hex_p(int index,short* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame)
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%06.4hx ",*data++);
            fprintf(pfile[index],"%6.4hx ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_chroma(int index,short* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_chroma_p(int index,short* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0 || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame  ||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_hex_chroma(int index,short* data,int width,int height,int stride)
{
    if(index>max_entry || index<0 ||!pfile[index] || !data || !width || !height  )
        return ;
    
    if(log_cur_frame<log_start_frame)
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%06.4hx ",*data++);
            fprintf(pfile[index],"%6.4hx ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_short_hex_chroma_p(int index,short* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
//            fprintf(pfile[index],"%06.4hx ",*data++);
            fprintf(pfile[index],"%6.4hx ",*data++);
            data++;
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_int(int index,int* data,int width,int height,int stride)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;
       
    if(log_cur_frame<log_start_frame ||!pfile[index])
        return;

    if(log_cur_frame>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}

void log_text_rect_int_p(int index,int* data,int width,int height,int stride,int fcnt)
{
    if(index>max_entry || index<0  || !data || !width || !height  )
        return ;
    
    index+=(fcnt%5)*max_entry;
    
    if(fcnt<log_start_frame ||!pfile[index])
        return;

    if(fcnt>log_end_frame)
    {
        log_closefile(index);
        return;
    }   

    int i=0,j=0;
    for(j=0;j<height;j++)
    {
        for(i=0;i<width;i++)
        {
            fprintf(pfile[index],"%6d  ",*data++);
        }
        fprintf(pfile[index],"\n");
        data+=stride;
    }
}





