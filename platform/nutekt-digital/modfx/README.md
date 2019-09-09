## Modulation Effect Template Project 

This project can be used as a basis for custom modulation effects. **Refer to [parent platform](../) for build instructions.**

### FAQ

#### Why does MODFX_PROCESS() have *main* and *sub* inputs and outputs?

The modulation effect API was originally designed for the prologue synthesizer which allows each *main* and *sub* timbre audio to be processed independently by the modulation effect section.
This interface was maintained in order to preserve API compatibility between the minilogue xd and prologue. 
For an effect to properly work on the prologue, both inputs should be processed in the same way and the result written to the corresponding output.

