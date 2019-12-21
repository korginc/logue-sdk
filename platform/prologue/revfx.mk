# #############################################################################
# Prologue Reverb FX Customization
# #############################################################################

PKGEXT = prlgunit

MCSRC = revfx_unit.c

MLDSCRIPT = userrevfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_prologue -DUSER_TARGET_MODULE=k_user_module_revfx

MSYMS = fx_api.syms
