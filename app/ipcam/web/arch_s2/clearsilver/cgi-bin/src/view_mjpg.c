#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_LEN (512)
#define NAME_LEN (128)
#define READ_INTERVAL (70)
const char* filename = "/tmp/snap0.jpg";

static int outputfile() {
    int fSize = 0;
    int bSize = 4096;
    FILE* file;
    struct stat filestat;

    file = fopen(filename, "r");
    if(!file){
        return -1;
    }
    stat(filename, &filestat);
    fSize = filestat.st_size;

    fprintf(stdout, "----AmbarellaBoundaryS0JIa4uHF7yHd8xJ\r\nContent-Type: image/jpeg\r\n\r\n");

    int chunk = fSize / bSize;
    int lbSize = fSize % bSize;
    void* buf = malloc(bSize);
    if(buf == NULL) {
        return -1;
    }
    //open file for writing

    int j;
    for(j = 0; j < chunk; j++){
        if(fread(buf, bSize, 1, file)){
            if(!fwrite(buf, bSize, 1, stdout)){
                free(buf);
                buf = NULL;
                return -1;
            }
        } else{
            free(buf);
            buf = NULL;
            return -1;
        }
    }
    free(buf);
    buf = NULL;
    if(lbSize){
        buf = malloc(lbSize);
        if(buf == NULL) {
            return -1;
        }
        if(fread(buf, lbSize, 1, file)){
            if(!fwrite(buf, lbSize, 1, stdout)){
                free(buf);
                buf = NULL;
                return -1;
            }
        }else{
            free(buf);
            buf = NULL;
            return -1;
        }
        free(buf);
        buf = NULL;
    }
    fprintf(stdout, "\r\n ");
    fflush(stdout);
    if(file){
      fclose(file);
    }

    return 0;
}

int main()
{
    fprintf(stdout, "Content-type: multipart/x-mixed-replace; boundary=----AmbarellaBoundaryS0JIa4uHF7yHd8xJ\r\n\r\n");
    while(1) {
        outputfile();
        usleep(READ_INTERVAL*1000);
    }
    return 0;

}


