#ifndef AUDIO_FUNCTIONS_H
#define AUDIO_FUNCTIONS_H

#include <Arduino.h>

void setupAudio();
void playFile(const char *filename);
void playNumberSequence(String numStr);
void playDigit(int digit);
void setVolume(float volume);

#endif