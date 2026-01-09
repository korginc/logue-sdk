  /**
   *  Existing API versions
   *
   *  Major: breaking changes (7bits, cap to 99)
   *  Minor: additions only   (7bits, cap to 99)
   *  Sub:   bugfixes only    (7bits, cap to 99)
   */
enum {
  k_unit_api_1_0_0 = ((1U << 16) | (0U << 8) | (0U)),
  k_unit_api_1_1_0 = ((1U << 16) | (1U << 8) | (0U)),
  k_unit_api_2_0_0 = ((2U << 16) | (0U << 8) | (0U)),
  k_unit_api_2_1_0 = ((2U << 16) | (1U << 8) | (0U))
};
  
  /**
   * Unit module categories.
   */
  enum {
    /** Dummy category, may be used in future. */
    k_unit_module_global = 0U,
    /** Modulation effects */
    k_unit_module_modfx,
    /** Delay effects */
    k_unit_module_delfx,
    /** Reverb effects */
    k_unit_module_revfx,
    /** Oscillators */
    k_unit_module_osc,
    /** Synth voices (drumlogue) */
    k_unit_module_synth,
    /** Master effects (drumlogue) */
    k_unit_module_masterfx,
    /** Generic effects, see: NTS-3 kaoss pad kit */
    k_unit_module_genericfx,
    k_num_unit_modules,
  };

  /**
   * prologue specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_prologue        = (1U<<8),
    k_unit_target_prologue_global = k_unit_target_prologue | k_unit_module_global,
    k_unit_target_prologue_modfx  = k_unit_target_prologue | k_unit_module_modfx,
    k_unit_target_prologue_delfx  = k_unit_target_prologue | k_unit_module_delfx,
    k_unit_target_prologue_revfx  = k_unit_target_prologue | k_unit_module_revfx,
    k_unit_target_prologue_osc    = k_unit_target_prologue | k_unit_module_osc,
  };

  /**
   * minilogue-xd specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_miniloguexd        = (2U<<8),
    k_unit_target_miniloguexd_global = k_unit_target_miniloguexd | k_unit_module_global,
    k_unit_target_miniloguexd_modfx  = k_unit_target_miniloguexd | k_unit_module_modfx,
    k_unit_target_miniloguexd_delfx  = k_unit_target_miniloguexd | k_unit_module_delfx,
    k_unit_target_miniloguexd_revfx  = k_unit_target_miniloguexd | k_unit_module_revfx,
    k_unit_target_miniloguexd_osc    = k_unit_target_miniloguexd | k_unit_module_osc,
  };

  /**
   * Nu:Tekt NTS-1 digital kit specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nutektdigital        = (3U<<8),
    k_unit_target_nutektdigital_global = k_unit_target_nutektdigital | k_unit_module_global,
    k_unit_target_nutektdigital_modfx  = k_unit_target_nutektdigital | k_unit_module_modfx,
    k_unit_target_nutektdigital_delfx  = k_unit_target_nutektdigital | k_unit_module_delfx,
    k_unit_target_nutektdigital_revfx  = k_unit_target_nutektdigital | k_unit_module_revfx,
    k_unit_target_nutektdigital_osc    = k_unit_target_nutektdigital | k_unit_module_osc,
  };

  /**
   * Aliases for Nu:Tekt NTS-1 digital kit specific platform/module pairs.
   */
  enum {
    k_unit_target_nts1        = k_unit_target_nutektdigital,
    k_unit_target_nts1_global = k_unit_target_nutektdigital_global,
    k_unit_target_nts1_modfx  = k_unit_target_nutektdigital_modfx,
    k_unit_target_nts1_delfx  = k_unit_target_nutektdigital_delfx,
    k_unit_target_nts1_revfx  = k_unit_target_nutektdigital_revfx,
    k_unit_target_nts1_osc    = k_unit_target_nutektdigital_osc,
  };

  /**
   * drumlogue specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_drumlogue          = (4U << 8),
    k_unit_target_drumlogue_delfx    = k_unit_target_drumlogue | k_unit_module_delfx,
    k_unit_target_drumlogue_revfx    = k_unit_target_drumlogue | k_unit_module_revfx,
    k_unit_target_drumlogue_synth    = k_unit_target_drumlogue | k_unit_module_synth,
    k_unit_target_drumlogue_masterfx = k_unit_target_drumlogue | k_unit_module_masterfx,
  };

  /**
   * Nu:Tekt NTS-1 digital kit mkII specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nts1_mkii        = (5U<<8),
    k_unit_target_nts1_mkii_global = k_unit_target_nts1_mkii | k_unit_module_global,
    k_unit_target_nts1_mkii_modfx  = k_unit_target_nts1_mkii | k_unit_module_modfx,
    k_unit_target_nts1_mkii_delfx  = k_unit_target_nts1_mkii | k_unit_module_delfx,
    k_unit_target_nts1_mkii_revfx  = k_unit_target_nts1_mkii | k_unit_module_revfx,
    k_unit_target_nts1_mkii_osc    = k_unit_target_nts1_mkii | k_unit_module_osc,
  };

  /**
   * Nu:Tekt NTS-3 kaoss pad kit specific platform/module pairs.
   * Passed to user code via initialization callback.
   */
  enum {
    k_unit_target_nts3_kaoss           = (6U<<8),
    k_unit_target_nts3_kaoss_global    = k_unit_target_nts3_kaoss | k_unit_module_global,
    k_unit_target_nts3_kaoss_genericfx = k_unit_target_nts3_kaoss | k_unit_module_genericfx,
  };

  /**
   * microkorg2 specific platform/module pairs. 
   * Passed to user code via initialization callback.
   */
enum {
    k_unit_target_microkorg2 = (7U << 8),
    k_unit_target_microkorg2_modfx = k_unit_target_microkorg2 | k_unit_module_modfx,
    k_unit_target_microkorg2_delfx = k_unit_target_microkorg2 | k_unit_module_delfx,
    k_unit_target_microkorg2_revfx = k_unit_target_microkorg2 | k_unit_module_revfx,
    k_unit_target_microkorg2_osc   = k_unit_target_microkorg2 | k_unit_module_osc,
  };