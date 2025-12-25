#include "ripplerx.h"
#include "constants.h"
/* RipplerX */
//FOR PORTING this handles polyphony
size_t RipplerX::nextVoiceNumber()
{
    size_t longestFramesSinceNoteOn = 0;
    size_t bestVoice = 0;

    for (size_t i = 0; i < c_numVoices; ++i) {
        Voice& voice = *voices[i];
        size_t thisVoiceFramesSinceNoteOn = voice.getFramesSinceNoteOn();
        if (thisVoiceFramesSinceNoteOn > longestFramesSinceNoteOn)
        {
            longestFramesSinceNoteOn = thisVoiceFramesSinceNoteOn;
            bestVoice = i;
        }
    }

    return bestVoice;
}


// this is pointed by header.c parameter
const char* const RipplerX::c_sampleBankName[c_sampleBankElements] = {
  "CH",
  "OH",
  "RS",
  "CP",
  "MISC",
  "USER",
  "EXP"
};


// this is pointed by header.c parameter
const char* const RipplerX::c_modelName[c_modelElements] = {
    "String",
    "Beam",
    "Squared",
    "Membrane",
    "Plate",
    "Drumhead",
    "Marimba",
    "Open Tube",
    "Closed Tube"
};

// this is pointed by header.c getParameter
const char* const RipplerX::c_partialsName[c_partialElements] = {
    "4", "8", "16", "32", "64"
};
const int RipplerX::c_partials[c_partialElements] = {
    4, 8, 16, 32, 64
};

// this is pointed by header.c parameter
const char* const RipplerX::c_noiseFilterModeName[c_noiseFilterModeElements] = {
    "LP",
    "BP",
    "HP"
};


// this is pointed by header.c parameter
const char* const RipplerX::c_programName[Program::last_program] = {
  "Bells",
  "Bells2",
  "Bong",
  "Cans",
  "Crash",
  "Crystal",
  "DoorBell",
  "Fifths",
  "Fight",
  "Flute",
  "Gong",
  "Harp",
  "Harpsi",
  "Init",
  "Kalimba",
  "KeyRing",
  "Marimba",
  "OldClock",
  "Ride",
  "Ride2",
  "Sankyo",
  "Sink",
  "Stars",
  "Strings",
  "Tabla",
  "Tabla2",
  "Tubes",
  "Vibes"
};

