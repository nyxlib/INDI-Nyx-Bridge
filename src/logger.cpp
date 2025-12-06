/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#include <cstdarg>

#include <libindi/indilogger.h>

#include "external/mongoose.h"

/*--------------------------------------------------------------------------------------------------------------------*/

int nyx_curr_log_level = MG_LL_NONE; /* NOSONAR */

/*--------------------------------------------------------------------------------------------------------------------*/

void __attribute__((format(printf, 1, 2))) nyx_log(const char *fmt, ...)
{
    if(nyx_curr_log_level <= mg_log_level)
    {
        std::string buff;

        buff.resize(4096);

        /*------------------------------------------------------------------------------------------------------------*/

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buff.data(), buff.size(), fmt, ap);
        va_end(ap);

        /*------------------------------------------------------------------------------------------------------------*/

        switch(nyx_curr_log_level)
        {
            case MG_LL_ERROR:   IDLog("[Bridge][ERROR] %s\n", buff.c_str());   break;
            case MG_LL_INFO:    IDLog("[Bridge][INFO] %s\n", buff.c_str());    break;
            case MG_LL_DEBUG:   IDLog("[Bridge][DEBUG] %s\n", buff.c_str());   break;
            case MG_LL_VERBOSE: IDLog("[Bridge][VERBOSE] %s\n", buff.c_str()); break;
            default:
                break;
        }

        /*------------------------------------------------------------------------------------------------------------*/
    }
}

/*--------------------------------------------------------------------------------------------------------------------*/
