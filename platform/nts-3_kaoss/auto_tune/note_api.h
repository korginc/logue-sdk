#ifndef NOTE_API_H
#define NOTE_API_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define NOTE_MIN   0
#define NOTE_MAX 128

static const char *rootNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

typedef enum {
    CHROMATIC,
    IONIAN,   // Major
    AEOLIAN,  // Minor
    DORIAN,
    LYDIAN,
    MIXOLYDIAN,
    PHRYGIAN,
    MAJOR_PENTATONIC,
    MINOR_PENTATONIC,
    SCALE_MAX
} ScaleType;

static const char *scaleNames[SCALE_MAX] = {
    "CHROMA",
    "MAJOR",
    "MINOR",
    "DORIAN",
    "LYDIAN",
    "MIXOLY",
    "PHRYG",
    "MJ PEN",
    "MI PEN"
};

static uint8_t chromaticScale[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static uint8_t ionianScale[] = {0, 2, 4, 5, 7, 9, 11};
static uint8_t aeolianScale[] = {0, 2, 3, 5, 7, 8, 10};
static uint8_t dorianScale[] = {0, 2, 3, 5, 7, 9, 10};
static uint8_t lydianScale[] = {0, 2, 4, 6, 7, 9, 11};
static uint8_t mixolydianScale[] = {0, 2, 4, 5, 7, 9, 10};
static uint8_t phrygianScale[] = {0, 1, 3, 5, 7, 8, 10};
static uint8_t majorPentatonicScale[] = {0, 2, 4, 7, 9};
static uint8_t minorPentatonicScale[] = {0, 3, 5, 7, 10};

static void getScaleOffsets(ScaleType scale, uint8_t **scaleOffsets, uint8_t *scaleSize) {
    switch (scale) {
        case CHROMATIC:
            *scaleOffsets = chromaticScale;
            *scaleSize = sizeof(chromaticScale) / sizeof(uint8_t);
            break;
        case IONIAN:
            *scaleOffsets = ionianScale;
            *scaleSize = sizeof(ionianScale) / sizeof(uint8_t);
            break;
        case AEOLIAN:
            *scaleOffsets = aeolianScale;
            *scaleSize = sizeof(aeolianScale) / sizeof(uint8_t);
            break;
        case DORIAN:
            *scaleOffsets = dorianScale;
            *scaleSize = sizeof(dorianScale) / sizeof(uint8_t);
            break;
        case LYDIAN:
            *scaleOffsets = lydianScale;
            *scaleSize = sizeof(lydianScale) / sizeof(uint8_t);
            break;
        case MIXOLYDIAN:
            *scaleOffsets = mixolydianScale;
            *scaleSize = sizeof(mixolydianScale) / sizeof(uint8_t);
            break;
        case PHRYGIAN:
            *scaleOffsets = phrygianScale;
            *scaleSize = sizeof(phrygianScale) / sizeof(uint8_t);
            break;
        case MAJOR_PENTATONIC:
            *scaleOffsets = majorPentatonicScale;
            *scaleSize = sizeof(majorPentatonicScale) / sizeof(uint8_t);
            break;
        case MINOR_PENTATONIC:
            *scaleOffsets = minorPentatonicScale;
            *scaleSize = sizeof(minorPentatonicScale) / sizeof(uint8_t);
            break;
        default:
            *scaleOffsets = chromaticScale;
            *scaleSize = sizeof(chromaticScale) / sizeof(uint8_t);
            break;
    }
}

static uint8_t quantizeToScale(uint8_t midiNote, ScaleType scaleType, uint8_t key) {
    uint8_t *scaleOffsets;
    uint8_t scaleSize;
    getScaleOffsets(scaleType, &scaleOffsets, &scaleSize);

    uint8_t octave = midiNote / 12;
    int8_t noteInOctave = (int8_t)(midiNote % 12);

    // Find closest scale note
    uint8_t closestNote = (scaleOffsets[0] + key) % 12;
    int8_t minDistance = abs(noteInOctave - (int8_t)closestNote);
    
    // Handle wrap-around distance (e.g. B to C)
    if (minDistance > 6) minDistance = 12 - minDistance;

    for (uint8_t i = 1; i < scaleSize; i++) {
        uint8_t currentNote = (scaleOffsets[i] + key) % 12;
        int8_t distance = abs(noteInOctave - (int8_t)currentNote);
        if (distance > 6) distance = 12 - distance;
        
        if (distance < minDistance) {
            minDistance = distance;
            closestNote = currentNote;
        }
    }

    return (uint8_t)(octave * 12 + closestNote);
}

#endif // NOTE_API_H
