# #############################################################################
# Minilogue XD Reverb FX Customization
# #############################################################################

PKGEXT = mnlgxdunit

MCSRC = revfx_unit.c

MLDSCRIPT = userrevfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_miniloguexd -DUSER_TARGET_MODULE=k_user_module_revfx

MSYMS = fx_api.syms
