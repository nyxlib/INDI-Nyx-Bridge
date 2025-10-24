/* INDI-Nyx Driver
 * Author: Jérôme ODIER <jerome.odier@lpsc.in2p3.fr>
 * SPDX-License-Identifier: GPL-2.0-only
 */

/*--------------------------------------------------------------------------------------------------------------------*/

#pragma once

#include <atomic>
#include <thread>

#include <libindi/defaultdevice.h>

/*--------------------------------------------------------------------------------------------------------------------*/

class IndiNyxDriver : public INDI::DefaultDevice
{
public:
    /*----------------------------------------------------------------------------------------------------------------*/

    IndiNyxDriver();
    ~IndiNyxDriver() override;

    const char *getDefaultName() override;

    /*----------------------------------------------------------------------------------------------------------------*/

    bool initProperties() override;

    void ISGetProperties(const char *dev) override;

    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

    /*----------------------------------------------------------------------------------------------------------------*/

    bool saveConfigItems(FILE *fp) override;

    /*----------------------------------------------------------------------------------------------------------------*/

private:
    /*----------------------------------------------------------------------------------------------------------------*/

    IText               INDISettingsT[1]{};
    ITextVectorProperty INDISettingsTP{};

    IText               MQTTSettingsT[3]{};
    ITextVectorProperty MQTTSettingsTP{};

    /*----------------------------------------------------------------------------------------------------------------*/

    std::atomic<bool> m_WorkerRunning{false};

    std::thread m_WorkerThread;

    void workerThreadFunc();

    /*----------------------------------------------------------------------------------------------------------------*/
};

/*--------------------------------------------------------------------------------------------------------------------*/
