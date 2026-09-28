// AEq -- Equalizer plugin for ALSA
// Copyright 2010 John Lindgren <john.lindgren@aol.com>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions, and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions, and the following disclaimer in the documentation
//    provided with the distribution.
//
// This software is provided "as is" and without any warranty, express or
// implied. In no event shall the authors be liable for any damages arising from
// the use of this software.

#include "common.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <libiniparser/iniparser.h>

// Q value for band-pass filters 1.2247 = (3/2)^(1/2)
// Gives 4 dB suppression at Fc*2 and Fc/2
int bands_num = BANDS;
float freqs[BANDS] = {31.25f, 62.5f, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
float Q[BANDS] = {1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f, 1.224745f};
int only_channel0 = 0;

void read_config (const char * path, int *on, float bands[BANDS + 1]) {
    int i;
    char tmp[20] = {};
    dictionary *dic;

    memset (bands, 0, (BANDS + 1) * sizeof bands[0]);

    dic = iniparser_load(path);
    if (dic == NULL) {
      FAIL ("read from", path);
      *on = 0;
      return;
    }

    only_channel0 = iniparser_getint(dic, "aeq:only_channel0", 0);

    *on = iniparser_getint(dic, "aeq:enable", 0);
    if(*on) {
        bands_num = iniparser_getint(dic, "aeq:bands", BANDS);
        if(bands_num < 0 || bands_num > BANDS)
            bands_num = BANDS;

        for (i = 0; i < bands_num; i++) {
            snprintf(tmp, sizeof(tmp), "aeq:freq%d", i);
            freqs[i] = iniparser_getdouble(dic, tmp, freqs[i]);

            snprintf(tmp, sizeof(tmp), "aeq:band%d", i);
            bands[i] = iniparser_getdouble(dic, tmp, bands[i]);

            snprintf(tmp, sizeof(tmp), "aeq:Q%d", i);
            Q[i] = iniparser_getdouble(dic, tmp, Q[i]);
        }

        bands[BANDS] = iniparser_getdouble(dic, "aeq:preamp_band", 0);
    }

    iniparser_freedict(dic);
    return;
}
