/**
 * Reset the FDN engine state
 * Call this when host calls unit_reset()
 */
void reset() {
    // Clear delay lines
    if (fdnMem != nullptr) {
        memset(fdnMem, 0, FDN_BUFFER_SIZE * FDN_CHANNELS * sizeof(float));
    }

    // Reset write position
    writePos = 0;

    // Reset modulation phases
    for (int i = 0; i < FDN_CHANNELS; i++) {
        modPhases[i] = 0.0f;
    }

    // Reset state
    memset(fdnState, 0, sizeof(fdnState));

    // Reset color LPF states
    colorLpfL = 0.0f;
    colorLpfR = 0.0f;
}