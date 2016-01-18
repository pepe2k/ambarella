//
typedef  float    coef_type;
typedef  short   input_coef_type;

//PI
static const coef_type c_pi= 3.14159265358979;
// squrt(2)
static const coef_type c_sq2=1.414213562373095;
//cos(i*PI/16)*squrt(2)
static const coef_type c_c0= c_sq2;
static const coef_type c_c1= 1.387039845322147;
static const coef_type c_c2= 1.306562964876377;
static const coef_type c_c3= 1.175875602419359;
static const coef_type c_c4= 1.0;
static const coef_type c_c5= 0.7856949583871022;
static const coef_type c_c6= 0.5411961001461970;
static const coef_type c_c7= 0.2758993792829430;

static const coef_type rounding=0.5;

#if 0
/* pixel operations */
#define MAX_NEG_CROP 1024

/* temporary */
static uint8_t ff_cropTbl[256 + 2 * MAX_NEG_CROP];

void init_ff_cropTbl()
{
    int i=0;
    for(i=0;i<256;i++)
    {
        ff_cropTbl[i + MAX_NEG_CROP] = i;
    }
    for(i=0;i<MAX_NEG_CROP;i++)
    {
        ff_cropTbl[i] = 0;
        ff_cropTbl[i + MAX_NEG_CROP + 256] = 255;
    }
}
#endif

void f_idct_put (uint8_t *dest, int line_size,input_coef_type * row)
{
    coef_type m[8][8]={0};
    int i=0;
    coef_type a0, a1, a2, a3, b0, b1, b2, b3;
    coef_type t1,t2;
    input_coef_type * pcrow=row;
    coef_type* pa=m;
    uint32_t temp;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    //row transform:
    for(i=0;i<8;i++,pcrow+=8,pa+=8)
    {
        if (!(((uint32_t*)pcrow)[1] |
              ((uint32_t*)pcrow)[2] |
              ((uint32_t*)pcrow)[3] |
              pcrow[1])) 
        {
            temp = pcrow[0] & 0xffff;
            temp |= temp << 16;
            ((uint32_t*)pcrow)[0]=((uint32_t*)pcrow)[1] =
            ((uint32_t*)pcrow)[2]=((uint32_t*)pcrow)[3] = temp;
            continue;
        }
            
        a0=c_c4*pcrow[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pcrow[2];
        t2=c_c6*pcrow[2];
        a0+=t1;
        a1+=t2;
        a2-=t2;
        a3-=t1;

        t1=pcrow[1];
        t2=pcrow[3];
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        temp = ((uint32_t*)pcrow)[2] | ((uint32_t*)pcrow)[3];

        if(temp)
        {
            t1=pcrow[4];
            t2=pcrow[6];

            a0+=c_c4*t1+c_c6*t2;
            a1-=c_c4*t1+c_c2*t2;
            a2+=c_c2*t2-c_c4*t1;
            a3+=c_c4*t1-c_c6*t2;

            t1=pcrow[5];
            t2=pcrow[7];
            b0+=c_c5*t1+c_c7*t2;
            b1-=c_c1*t1+c_c5*t2;
            b2+=c_c7*t1+c_c3*t2;
            b3+=c_c3*t1-c_c1*t2;
        }

        pa[0]=a0+b0;
        pa[7]=a0-b0;
        pa[1]=a1+b1;
        pa[6]=a1-b1;
        pa[2]=a2+b2;
        pa[5]=a2-b2;
        pa[3]=a3+b3;
        pa[4]=a3-b3;
        
    }
    
    //column transform
    pa=m;
    for(i=0;i<8;i++,pa++)
    {
        a0=c_c4*pa[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pa[16];
        t2=c_c6*pa[16];

        a0+=t1;
        a1+=t2;
        a2-=t1;
        a3-=t2;

        t1=pa[8];
        t2=pa[24];
        
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        t1=pa[32]*c_c4;
        t2=pa[40];

        a0+=t1;
        a1-=t1;
        a2-=t1;
        a3+=t1;

        t1=c_c6*pa[48];
        
        b0+=c_c5*t2;
        b1-=c_c1*t2;
        b2+=c_c7*t2;
        b3+=c_c3*t2;

        t2=c_c2*pa[48];
        
        a0+=t1;
        a1-=t2;
        a2+=t2;
        a3-=t1;

        t2=pa[56];

        b0+=c_c7*t2;
        b1-=c_c5*t2;
        b2+=c_c3*t2;
        b3-=c_c1*t2;

        dest[i]                    = cm[((int)(a0+b0+rounding))>>3];
        dest[i+line_size]     = cm[((int)(a1+b1+rounding))>>3];
        dest[i+line_size*2] = cm[((int)(a2+b2+rounding))>>3];
        dest[i+line_size*3] = cm[((int)(a3+b3+rounding))>>3];
        dest[i+line_size*4] = cm[((int)(a3-b3+rounding))>>3];
        dest[i+line_size*5] = cm[((int)(a2-b2+rounding))>>3];
        dest[i+line_size*6] = cm[((int)(a1-b1+rounding))>>3];
        dest[i+line_size*7] = cm[((int)(a0-b0+rounding))>>3];
    }
}

void f_idct_add (uint8_t *dest, int line_size,input_coef_type * row)
{
    coef_type m[8][8]={0};
    int i=0;
    coef_type a0, a1, a2, a3, b0, b1, b2, b3;
    coef_type t1,t2;
    input_coef_type * pcrow=row;
    coef_type* pa=m;
    uint32_t temp;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    //row transform:
    for(i=0;i<8;i++,pcrow+=8,pa+=8)
    {
        if (!(((uint32_t*)pcrow)[1] |
              ((uint32_t*)pcrow)[2] |
              ((uint32_t*)pcrow)[3] |
              pcrow[1])) 
        {
            temp = pcrow[0] & 0xffff;
            temp |= temp << 16;
            ((uint32_t*)pcrow)[0]=((uint32_t*)pcrow)[1] =
            ((uint32_t*)pcrow)[2]=((uint32_t*)pcrow)[3] = temp;
            continue;
        }
            
        a0=c_c4*pcrow[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pcrow[2];
        t2=c_c6*pcrow[2];
        a0+=t1;
        a1+=t2;
        a2-=t2;
        a3-=t1;

        t1=pcrow[1];
        t2=pcrow[3];
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        temp = ((uint32_t*)pcrow)[2] | ((uint32_t*)pcrow)[3];

        if(temp)
        {
            t1=pcrow[4];
            t2=pcrow[6];

            a0+=c_c4*t1+c_c6*t2;
            a1-=c_c4*t1+c_c2*t2;
            a2+=c_c2*t2-c_c4*t1;
            a3+=c_c4*t1-c_c6*t2;

            t1=pcrow[5];
            t2=pcrow[7];
            b0+=c_c5*t1+c_c7*t2;
            b1-=c_c1*t1+c_c5*t2;
            b2+=c_c7*t1+c_c3*t2;
            b3+=c_c3*t1-c_c1*t2;
        }

        pa[0]=a0+b0;
        pa[7]=a0-b0;
        pa[1]=a1+b1;
        pa[6]=a1-b1;
        pa[2]=a2+b2;
        pa[5]=a2-b2;
        pa[3]=a3+b3;
        pa[4]=a3-b3;
        
    }
    
    //column transform
    pa=m;
    for(i=0;i<8;i++,pa++)
    {
        a0=c_c4*pa[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pa[16];
        t2=c_c6*pa[16];

        a0+=t1;
        a1+=t2;
        a2-=t1;
        a3-=t2;

        t1=pa[8];
        t2=pa[24];
        
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        t1=pa[32]*c_c4;
        t2=pa[40];

        a0+=t1;
        a1-=t1;
        a2-=t1;
        a3+=t1;

        t1=c_c6*pa[48];
        
        b0+=c_c5*t2;
        b1-=c_c1*t2;
        b2+=c_c7*t2;
        b3+=c_c3*t2;

        t2=c_c2*pa[48];
        
        a0+=t1;
        a1-=t2;
        a2+=t2;
        a3-=t1;

        t2=pa[56];

        b0+=c_c7*t2;
        b1-=c_c5*t2;
        b2+=c_c3*t2;
        b3-=c_c1*t2;

        dest[i]                    = cm[dest[i]+(((int)(a0+b0+rounding))>>3)];
        dest[i+line_size]     = cm[dest[i+line_size]+(((int)(a1+b1+rounding))>>3)];
        dest[i+line_size*2] = cm[dest[i+line_size*2]+(((int)(a2+b2+rounding))>>3)];
        dest[i+line_size*3] = cm[dest[i+line_size*3]+(((int)(a3+b3+rounding))>>3)];
        dest[i+line_size*4] = cm[dest[i+line_size*4]+(((int)(a3-b3+rounding))>>3)];
        dest[i+line_size*5] = cm[dest[i+line_size*5]+(((int)(a2-b2+rounding))>>3)];
        dest[i+line_size*6] = cm[dest[i+line_size*6]+(((int)(a1-b1+rounding))>>3)];
        dest[i+line_size*7] = cm[dest[i+line_size*7]+(((int)(a0-b0+rounding))>>3)];
    }
}

void f_idct_put_nv12 (uint8_t *dest, int line_size,input_coef_type * row)
{
    coef_type m[8][8]={0};
    int i=0;
    coef_type a0, a1, a2, a3, b0, b1, b2, b3;
    coef_type t1,t2;
    input_coef_type * pcrow=row;
    coef_type* pa=m;
    uint32_t temp;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    //row transform:
    for(i=0;i<8;i++,pcrow+=8,pa+=8)
    {
        if (!(((uint32_t*)pcrow)[1] |
              ((uint32_t*)pcrow)[2] |
              ((uint32_t*)pcrow)[3] |
              pcrow[1])) 
        {
            temp = pcrow[0] & 0xffff;
            temp |= temp << 16;
            ((uint32_t*)pcrow)[0]=((uint32_t*)pcrow)[1] =
            ((uint32_t*)pcrow)[2]=((uint32_t*)pcrow)[3] = temp;
            continue;
        }
            
        a0=c_c4*pcrow[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pcrow[2];
        t2=c_c6*pcrow[2];
        a0+=t1;
        a1+=t2;
        a2-=t2;
        a3-=t1;

        t1=pcrow[1];
        t2=pcrow[3];
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        temp = ((uint32_t*)pcrow)[2] | ((uint32_t*)pcrow)[3];

        if(temp)
        {
            t1=pcrow[4];
            t2=pcrow[6];

            a0+=c_c4*t1+c_c6*t2;
            a1-=c_c4*t1+c_c2*t2;
            a2+=c_c2*t2-c_c4*t1;
            a3+=c_c4*t1-c_c6*t2;

            t1=pcrow[5];
            t2=pcrow[7];
            b0+=c_c5*t1+c_c7*t2;
            b1-=c_c1*t1+c_c5*t2;
            b2+=c_c7*t1+c_c3*t2;
            b3+=c_c3*t1-c_c1*t2;
        }

        pa[0]=a0+b0;
        pa[7]=a0-b0;
        pa[1]=a1+b1;
        pa[6]=a1-b1;
        pa[2]=a2+b2;
        pa[5]=a2-b2;
        pa[3]=a3+b3;
        pa[4]=a3-b3;
        
    }
    
    //column transform
    pa=m;
    for(i=0;i<16;i+=2,pa++)
    {
        a0=c_c4*pa[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pa[16];
        t2=c_c6*pa[16];

        a0+=t1;
        a1+=t2;
        a2-=t1;
        a3-=t2;

        t1=pa[8];
        t2=pa[24];
        
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        t1=pa[32]*c_c4;
        t2=pa[40];

        a0+=t1;
        a1-=t1;
        a2-=t1;
        a3+=t1;

        t1=c_c6*pa[48];
        
        b0+=c_c5*t2;
        b1-=c_c1*t2;
        b2+=c_c7*t2;
        b3+=c_c3*t2;

        t2=c_c2*pa[48];
        
        a0+=t1;
        a1-=t2;
        a2+=t2;
        a3-=t1;

        t2=pa[56];

        b0+=c_c7*t2;
        b1-=c_c5*t2;
        b2+=c_c3*t2;
        b3-=c_c1*t2;

        dest[i]                    = cm[((int)(a0+b0+rounding))>>3];
        dest[i+line_size]     = cm[((int)(a1+b1+rounding))>>3];
        dest[i+line_size*2] = cm[((int)(a2+b2+rounding))>>3];
        dest[i+line_size*3] = cm[((int)(a3+b3+rounding))>>3];
        dest[i+line_size*4] = cm[((int)(a3-b3+rounding))>>3];
        dest[i+line_size*5] = cm[((int)(a2-b2+rounding))>>3];
        dest[i+line_size*6] = cm[((int)(a1-b1+rounding))>>3];
        dest[i+line_size*7] = cm[((int)(a0-b0+rounding))>>3];
    }
}

void f_idct_add_nv12 (uint8_t *dest, int line_size,input_coef_type * row)
{
    coef_type m[8][8]={0};
    int i=0;
    coef_type a0, a1, a2, a3, b0, b1, b2, b3;
    coef_type t1,t2;
    input_coef_type * pcrow=row;
    coef_type* pa=m;
    uint32_t temp;
    uint8_t *cm = ff_cropTbl + MAX_NEG_CROP;

    //row transform:
    for(i=0;i<8;i++,pcrow+=8,pa+=8)
    {
        if (!(((uint32_t*)pcrow)[1] |
              ((uint32_t*)pcrow)[2] |
              ((uint32_t*)pcrow)[3] |
              pcrow[1])) 
        {
            temp = pcrow[0] & 0xffff;
            temp |= temp << 16;
            ((uint32_t*)pcrow)[0]=((uint32_t*)pcrow)[1] =
            ((uint32_t*)pcrow)[2]=((uint32_t*)pcrow)[3] = temp;
            continue;
        }
            
        a0=c_c4*pcrow[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pcrow[2];
        t2=c_c6*pcrow[2];
        a0+=t1;
        a1+=t2;
        a2-=t2;
        a3-=t1;

        t1=pcrow[1];
        t2=pcrow[3];
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        temp = ((uint32_t*)pcrow)[2] | ((uint32_t*)pcrow)[3];

        if(temp)
        {
            t1=pcrow[4];
            t2=pcrow[6];

            a0+=c_c4*t1+c_c6*t2;
            a1-=c_c4*t1+c_c2*t2;
            a2+=c_c2*t2-c_c4*t1;
            a3+=c_c4*t1-c_c6*t2;

            t1=pcrow[5];
            t2=pcrow[7];
            b0+=c_c5*t1+c_c7*t2;
            b1-=c_c1*t1+c_c5*t2;
            b2+=c_c7*t1+c_c3*t2;
            b3+=c_c3*t1-c_c1*t2;
        }

        pa[0]=a0+b0;
        pa[7]=a0-b0;
        pa[1]=a1+b1;
        pa[6]=a1-b1;
        pa[2]=a2+b2;
        pa[5]=a2-b2;
        pa[3]=a3+b3;
        pa[4]=a3-b3;
        
    }
    
    //column transform
    pa=m;
    for(i=0;i<16;i+=2,pa++)
    {
        a0=c_c4*pa[0];
        a1=a0;
        a2=a0;
        a3=a0;

        t1=c_c2*pa[16];
        t2=c_c6*pa[16];

        a0+=t1;
        a1+=t2;
        a2-=t1;
        a3-=t2;

        t1=pa[8];
        t2=pa[24];
        
        b0=c_c1*t1+c_c3*t2;
        b1=c_c3*t1-c_c7*t2;
        b2=c_c5*t1-c_c1*t2;
        b3=c_c7*t1-c_c5*t2;

        t1=pa[32]*c_c4;
        t2=pa[40];

        a0+=t1;
        a1-=t1;
        a2-=t1;
        a3+=t1;

        t1=c_c6*pa[48];
        
        b0+=c_c5*t2;
        b1-=c_c1*t2;
        b2+=c_c7*t2;
        b3+=c_c3*t2;

        t2=c_c2*pa[48];
        
        a0+=t1;
        a1-=t2;
        a2+=t2;
        a3-=t1;

        t2=pa[56];

        b0+=c_c7*t2;
        b1-=c_c5*t2;
        b2+=c_c3*t2;
        b3-=c_c1*t2;

        dest[i]                    = cm[dest[i]+(((int)(a0+b0+rounding))>>3)];
        dest[i+line_size]     = cm[dest[i+line_size]+(((int)(a1+b1+rounding))>>3)];
        dest[i+line_size*2] = cm[dest[i+line_size*2]+(((int)(a2+b2+rounding))>>3)];
        dest[i+line_size*3] = cm[dest[i+line_size*3]+(((int)(a3+b3+rounding))>>3)];
        dest[i+line_size*4] = cm[dest[i+line_size*4]+(((int)(a3-b3+rounding))>>3)];
        dest[i+line_size*5] = cm[dest[i+line_size*5]+(((int)(a2-b2+rounding))>>3)];
        dest[i+line_size*6] = cm[dest[i+line_size*6]+(((int)(a1-b1+rounding))>>3)];
        dest[i+line_size*7] = cm[dest[i+line_size*7]+(((int)(a0-b0+rounding))>>3)];
    }
}

