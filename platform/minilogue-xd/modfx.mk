# #############################################################################
# Minilogue XD Mod. FX Customization
# #############################################################################

PKGEXT = mnlgxdunit

MCSRC = modfx_unit.c

MLDSCRIPT = usermodfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_miniloguexd -DUSER_TARGET_MODULE=k_user_module_modfx

MSYMS = fx_api.syms
