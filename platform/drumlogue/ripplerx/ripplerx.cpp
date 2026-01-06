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

