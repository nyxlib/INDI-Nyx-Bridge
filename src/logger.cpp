/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <libindi/indilogger.h>

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

int nyx_curr_log_level = MG_LL_NONE;

/*--------------------------------------------------------------------------------------------------------------------*/

void nyx_log(const char *fmt, ...)
{
    if(nyx_curr_log_level <= mg_log_level)
    {
        char buff[4096];

        /*------------------------------------------------------------------------------------------------------------*/

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buff, sizeof(buff), fmt, ap);
        va_end(ap);

        /*------------------------------------------------------------------------------------------------------------*/

        switch(nyx_curr_log_level)
        {
            case MG_LL_ERROR:   IDLog("[Bridge][ERROR] %s\n", buff);   break;
            case MG_LL_INFO:    IDLog("[Bridge][INFO] %s\n", buff);    break;
            case MG_LL_DEBUG:   IDLog("[Bridge][DEBUG] %s\n", buff);   break;
            case MG_LL_VERBOSE: IDLog("[Bridge][VERBOSE] %s\n", buff); break;
        }

        /*------------------------------------------------------------------------------------------------------------*/
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
