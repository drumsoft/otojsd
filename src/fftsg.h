#ifndef FFTSG_H
#define FFTSG_H

#define USE_CDFT_PTHREADS

void rdft(int n, int isgn, double *a, int *ip, double *w);

#endif /* FFTSG_H */
