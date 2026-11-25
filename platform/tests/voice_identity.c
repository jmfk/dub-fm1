#include "engine.h"
#include <stdio.h>
int main(void){a_init();for(int t=0;t<8;t++){ap[t].p[P_SLOPE]=t&1;a_note_on(t,42+t*3,90+t,1);a_note_on(t,49+t*3,88,1);}
for(int b=0;b<1400;b++){int32_t pcm[64]={0};if(b==100){ap[3].bend=6000;ap[2].mod=18000;}if(b==500)for(int t=0;t<8;t++)a_note_off(t,42+t*3,1);if(b==700)for(int t=0;t<8;t++)a_note_off(t,49+t*3,1);a_render(pcm,32);for(int t=0;t<8;t++)fwrite(a_track_buf[t],4,32,stdout);}return 0;}
