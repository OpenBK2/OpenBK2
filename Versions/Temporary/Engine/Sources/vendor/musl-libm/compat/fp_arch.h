/* musl's per-architecture fp_arch.h overrides fp_barrier and friends with
   inline assembly on some targets. Neither x86 nor x86-64 has one upstream, so
   the generic C versions in libm.h are what musl itself uses here too. */
