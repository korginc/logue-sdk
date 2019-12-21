# #############################################################################
# Minilogue XD Delay FX Customization
# #############################################################################

PKGEXT = mnlgxdunit

MCSRC = delfx_unit.c

MLDSCRIPT = userdelfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_miniloguexd -DUSER_TARGET_MODULE=k_user_module_delfx

MSYMS = fx_api.syms
