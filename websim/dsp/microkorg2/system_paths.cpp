// Host stand-ins for microKORG2 device filesystem paths.
//
// On hardware these point into the device's flash/looper storage; waves.h (and
// other microKORG2 units) include SystemPaths.h, which declares them. websim
// has no device filesystem, so we provide harmless empty paths. See
// WEBSIM_EXPANSION_PLAN.md §4.2.

extern "C" {
const char *looperPath = "";
const char *unitPath = "";
}
