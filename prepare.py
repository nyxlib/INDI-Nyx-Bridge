#!/usr/bin/env python3
########################################################################################################################

import re
import requests

########################################################################################################################

MONGOOSE_VERSION = '7.19'

########################################################################################################################

LOGGER = '''
extern int nyx_curr_log_level;

void nyx_log(const char *fmt, ...);

#define MG_LOG(level, args)                         \\
            do {                                    \\
                nyx_curr_log_level = (level);       \\
                nyx_log args;                       \\
            } while(0)
'''[1: ]

########################################################################################################################

def download_mongoose():

    for filename in ['mongoose.c', 'mongoose.h']:

        ################################################################################################################

        response = requests.get(f'https://raw.githubusercontent.com/cesanta/mongoose/{MONGOOSE_VERSION}/{filename}')

        ################################################################################################################

        if response.status_code == 200:

            with open(f'src/external/{filename}', 'wt') as f:

                f.write(re.sub(r'#if\s+MG_ENABLE_LOG.*?#endif', LOGGER, response.content.decode('UTF-8'), flags = re.DOTALL))

        else:

            raise IOError(f'Cannot download `{filename}`')

########################################################################################################################

if __name__ == '__main__':

    download_mongoose()

########################################################################################################################
