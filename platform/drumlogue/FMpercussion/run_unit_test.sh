# Run all tests
gcc -o test_all tests.h fm_perc_synth.c -lm -DTEST_ALL
./test_all

# Run only PRNG tests during development
gcc -o test_prng tests.h -lm -DTEST_PRNG_ONLY
./test_prng

# Run only stability test (16 beats)
gcc -o test_stability tests.h fm_perc_synth.c -lm -DTEST_STABILITY_ONLY
./test_stability

# Run with cycle counting (requires ARM hardware or emulator)
arm-linux-gnueabihf-gcc -o test_arm tests.h fm_perc_synth.c -lm -DTEST_ALL
./test_arm