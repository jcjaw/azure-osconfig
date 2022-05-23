// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TPM_H
#define TPM_H

#include <cstring>
#include <string>

#include <CommonUtils.h>
#include <Logging.h>
#include <Mmi.h>

#define TPM_LOGFILE "/var/log/osconfig_tpm.log"
#define TPM_ROLLEDLOGFILE "/var/log/osconfig_tpm.bak"

#define TPM "Tpm"
#define TPM_STATUS "tpmStatus"
#define TPM_VERSION "tpmVersion"
#define TPM_MANUFACTURER "tpmManufacturer"

class TpmLog
{
public:
    static OSCONFIG_LOG_HANDLE Get()
    {
        return m_logTpm;
    }

    static void OpenLog()
    {
        m_logTpm = ::OpenLog(TPM_LOGFILE, TPM_ROLLEDLOGFILE);
    }

    static void CloseLog()
    {
        ::CloseLog(&m_logTpm);
    }

private:
    static OSCONFIG_LOG_HANDLE m_logTpm;
};

class Tpm
{
public:
    enum Status
    {
        Unknown = 0,
        TpmDetected,
        TpmNotDetected
    };

    Tpm(const unsigned int maxPayloadSizeBytes);
    virtual ~Tpm();
    static int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
    int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    virtual std::string RunCommand(const char* command);
    Status GetStatus();
    std::string GetVersionFromCapabilitiesFile();
    std::string GetManufacturerFromCapabilitiesFile();
    void HexToText(std::string& s);
    void Trim(std::string& s);
    unsigned char Decode(char c);

    const unsigned int m_maxPayloadSizeBytes;
    bool m_hasCapabilitiesFile;
};

#endif // TPM_H