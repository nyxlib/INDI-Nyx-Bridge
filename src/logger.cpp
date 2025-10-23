/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <libindi/indilogger.h>

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

int mg_to_indi_log_level = MG_LL_NONE;

/*--------------------------------------------------------------------------------------------------------------------*/

void mg_to_indi_log_int(const char *fmt, ...)
{
    if(mg_to_indi_log_level <= mg_log_level)
    {
        char buff[4096];

        /*------------------------------------------------------------------------------------------------------------*/

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buff, sizeof(buff), fmt, ap);
        va_end(ap);

        /*------------------------------------------------------------------------------------------------------------*/

        switch(mg_to_indi_log_level)
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
