# #############################################################################
# NTS-1 digital kit Delay FX Customization
# #############################################################################

PKGEXT = ntkdigunit

MCSRC = delfx_unit.c

MLDSCRIPT = userdelfx.ld

MDEFS = -DSTM32F446xE -DUSER_TARGET_PLATFORM=k_user_target_nutektdigital -DUSER_TARGET_MODULE=k_user_module_delfx

MSYMS = fx_api.syms
