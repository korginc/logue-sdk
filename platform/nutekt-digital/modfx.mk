# #############################################################################
# NTS-1 digital kit Mod. FX Customization
# #############################################################################

PKGEXT = ntkdigunit

MCSRC = modfx_unit.c

MLDSCRIPT = usermodfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_nutektdigital -DUSER_TARGET_MODULE=k_user_module_modfx

MSYMS = fx_api.syms
