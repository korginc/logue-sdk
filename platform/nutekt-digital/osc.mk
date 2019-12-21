# #############################################################################
# NTS-1 digital kit Oscillator Customization
# #############################################################################

PKGEXT = ntkdigunit

MCSRC = osc_unit.c

MLDSCRIPT = userosc.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_nutektdigital -DUSER_TARGET_MODULE=k_user_module_osc

MSYMS = osc_api.syms
