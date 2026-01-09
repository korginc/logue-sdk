#include "ripplerx.h"
#include "constants.h"
/* RipplerX */
//FOR PORTING this handles polyphony and voice stealing
// Voice stealing strategy: Find the voice that has been idle for the longest time
// Priority: uninitialized voices > released voices > oldest pressed voice
size_t RipplerX::nextVoiceNumber()
{
	// Track best candidate with initial "least idle" state
	size_t bestVoice = 0;
	size_t longestFramesSinceNoteOn = 0;

	// Check ALL voices including voice 0
	for (size_t i = 0; i < c_numVoices; ++i) {
		if (!voices[i]) continue;  // Safety check

		size_t thisVoiceFramesSinceNoteOn = voices[i]->getFramesSinceNoteOn();

		// SIZE_MAX indicates uninitialized voice - prioritize these first
		// Otherwise, pick the voice idle for the longest time
		if (thisVoiceFramesSinceNoteOn > longestFramesSinceNoteOn) {
			longestFramesSinceNoteOn = thisVoiceFramesSinceNoteOn;
			bestVoice = i;
		}
	}

	return bestVoice;
}


