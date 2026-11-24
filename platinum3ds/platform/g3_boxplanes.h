#pragma once
#include <math.h>
/* Conservative support-plane test. Call prepare once per matrix/batch, then
 * test each immutable chunk box. The widened stereo/wide planes are supplied
 * by the existing renderer; no view dimensions are changed. */
struct FastBoxPlanes {
    float p[6][4], error[4];
    unsigned count;
    void prepare(const float *m,float wide,bool depth) {
        count=depth?6:4;
        for(unsigned k=0;k<4;k++) {
            p[0][k]= m[4*k]-wide*m[4*k+3];
            p[1][k]=-m[4*k]-wide*m[4*k+3];
            p[2][k]= m[4*k+1]-m[4*k+3];
            p[3][k]=-m[4*k+1]-m[4*k+3];
            p[4][k]= m[4*k+2]-m[4*k+3];
            p[5][k]=-m[4*k+2]-m[4*k+3];
            /* Upper bound on the old per-corner tolerance, with extra room
             * for reassociation and widened-plane coefficient rounding. */
            error[k]=(fabsf(m[4*k])+fabsf(m[4*k+1])+fabsf(m[4*k+2])+fabsf(m[4*k+3]))*(1.0f+fabsf(wide))*1.01e-4f;
        }
    }
    unsigned test(const float *b) const {
        float a[3];
        for(unsigned k=0;k<3;k++){float x=fabsf(b[k]),y=fabsf(b[k+3]);a[k]=x>y?x:y;}
        const float e=error[0]*a[0]+error[1]*a[1]+error[2]*a[2]+error[3]+2.02e-4f;
        unsigned mask=0;
        for(unsigned k=0;k<count;k++) {
            const float *q=p[k];
            const float x=b[q[0]<0?3:0],y=b[q[1]<0?4:1],z=b[q[2]<0?5:2];
            const float d=x*q[0]+y*q[1]+z*q[2]+q[3];
            if(d>e) mask|=1u<<k;
            else if(!(d< -e)) mask|=0x80000000u;
        }
        return mask;
    }
};
