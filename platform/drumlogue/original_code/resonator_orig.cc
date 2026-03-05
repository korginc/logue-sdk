#include "resonator.h"

#include <string.h>
#include "biquad.h"

/* Resonator */
//FOR PORTING this handles polyphony
size_t Resonator::nextVoiceNumber()
{
  size_t longestFramesSinceNoteOn = 0;
  size_t bestVoice = 0;

  for (size_t i = 1; i < c_numVoices; i++)
  {
    size_t thisVoiceFramesSinceNoteOn = m_voice[i].getFramesSinceNoteOn();
    if (thisVoiceFramesSinceNoteOn > longestFramesSinceNoteOn)
    {
      longestFramesSinceNoteOn = thisVoiceFramesSinceNoteOn;
      bestVoice = i;
    }
  }

  return bestVoice;
}

const char* const Resonator::c_sampleBankName[7] = {
  "CH",
  "OH",
  "RS",
  "CP",
  "MISC",
  "USER",
  "EXP"
};

const char* const Resonator::c_excitationFilterTypeName[6] = {
  // Same order as biquad.h Filter::Type enum.
  "None",
  "LP12",
  "HP12",
  "BP6",
  "LP6",
  "HP6",
};

const float Resonator::c_dispersionFilterRatios[c_dispersionStages] = {
  // High values cause low dispersion.
  10.4,
  4.9,
  2.2,
  0.8,
  // Low values cause high dispersion.
};
