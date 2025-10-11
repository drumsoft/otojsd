#ifndef AIFFRECORDER_H
#define AIFFRECORDER_H
// OtoPerl::aiffrecorder.c - sound recorder for OtoPerl.

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
	FILE *fh;
	uint32_t frames;
	int framesize;
	int channels;
	unsigned char *headers;
} AiffRecorder;

AiffRecorder *AiffRecorder_create(int channels, int bits, int sampleRate);
void AiffRecorder_destroy(AiffRecorder *self);
bool AiffRecorder_open(AiffRecorder *self, const char *path);
bool AiffRecorder_write32bit(AiffRecorder *self, const uint32_t *data, int frames);
bool AiffRecorder_close(AiffRecorder *self);

#endif