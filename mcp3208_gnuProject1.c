#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pigpiod_if2.h>

#define BAUD_RATE   1000000
#define LOOP    100 

int main(int argc, char* argv[])
{
    FILE* fp;
    int res;
    double j;
    
    int pi;
    int spi;
    uint16_t value;
    uint64_t sum = 0;
    uint16_t avg;
    int channel = 0;
    char snd_buf[3];
    char rcv_buf[LOOP][3];
    uint32_t start_tick, diff_tick;
    double startTime;

    if(argc == 2)
        channel = atoi(argv[1]);
        
    if((pi = pigpio_start(NULL, NULL)) < 0) {
        fprintf(stderr, "pigpio_start error\n"); 
        return 1;
    }

    if((spi = spi_open(pi, 0, BAUD_RATE, 0)) < 0) {
        fprintf(stderr, "spi_open error\n");
        return 2;
    }

    fp = fopen("luxData", "w");
    
    if(fp == NULL){
        printf("open fail!\n");
        return 1;
    }

    j = 0;
    startTime = time_time();

    while(1){
        if((time_time() - startTime) > 30) break;
        
        avg = sum = 0;
        snd_buf[0] = 0x18 | channel;

        start_tick = get_current_tick(pi);

        for(int i = 0 ; i < LOOP ; i++){
            spi_xfer(pi, spi, snd_buf, rcv_buf[i], 3);
            time_sleep(0.0007);
        }

        diff_tick = get_current_tick(pi) - start_tick;
        
        printf("diff_tick: %.3fs\n", diff_tick/(float)1000000);

        for(int i = 0 ; i < LOOP ; i++){
            value = ((rcv_buf[i][1] & 0x3f) << 8) | (rcv_buf[i][2] & 0xff);
            value = value >> 2;
            sum += value;
        }
        
        avg = sum / LOOP;
        fprintf(fp, "%.1f %c %.1f\n", j/10, ' ', 3.3*avg/4095); 

        printf("channel-%d : %4d %.1fv", channel, avg, 3.3*avg/4095);
        printf("\t%lld sps\n", 1000000LL * LOOP / diff_tick);
        
        j++;
    }

    res = fclose(fp);
    if(res !=0){
        printf("file not close");
        return 1;
    }

	spi_close(pi, spi);
	pigpio_stop(pi);
	return 0;
}
