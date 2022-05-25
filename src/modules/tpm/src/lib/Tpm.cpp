// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <algorithm>
#include <cstdio>
#include <regex>

#include <Tpm2Utils.h>

constexpr const char g_moduleInfo[] = R""""({
    "Name": "Tpm",
    "Description": "Provides functionality to remotely query the TPM on device",
    "Manufacturer": "Microsoft",
    "VersionMajor": 1,
    "VersionMinor": 0,
    "VersionInfo": "Nickel",
    "Components": ["Tpm"],
    "Lifetime": 1,
    "UserAccount": 0})"""";

const char* g_getTpmDetected = "ls -d /dev/tpm[0-9]";
const char* g_getTpmrmDetected = "ls -d /dev/tpm[r][m][0-9]";
const char* g_getTpmCapabilities = "cat /sys/class/tpm/tpm0/caps";
const char* g_tpmDetected = "/dev/tpm[rm]*[0-9]";
const char* g_tpmVersionFromCapabilitiesFile = "TCG\\s+version:\\s+";
const char* g_tpmManufacturerFromCapabilitiesFile = "Manufacturer:\\s+0x";
const char* g_tpmVersionFromDeviceFile = "\\d(.\\d)?";
const char* g_tpmManufacturerFromDeviceFile = "[\\w\\s]+";
const char* g_quotationCharacter = "\"";

OSCONFIG_LOG_HANDLE TpmLog::m_logTpm = nullptr;

Tpm::Tpm(const unsigned int maxPayloadSizeBytes) : m_maxPayloadSizeBytes(maxPayloadSizeBytes)
{
    m_hasCapabilitiesFile = true;
}

Tpm::~Tpm() {}

int Tpm::GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;

    if (nullptr == clientName)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid clientName");
        status = EINVAL;
    }
    else if (nullptr == payload)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payload");
        status = EINVAL;
    }
    else if (nullptr == payloadSizeBytes)
    {
        OsConfigLogError(TpmLog::Get(), "Invalid payloadSizeBytes");
        status = EINVAL;
    }
    else
    {
        std::size_t len = ARRAY_SIZE(g_moduleInfo) - 1;
        *payload = new (std::nothrow) char[len];
        if (nullptr == *payload)
        {
            OsConfigLogError(TpmLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
        else
        {
            std::memcpy(*payload, g_moduleInfo, len);
            *payloadSizeBytes = len;
        }
    }

    return status;
}

std::string Tpm::RunCommand(const char* command)
{
    char* textResult = nullptr;
    std::string commandOutput;

    int status = ExecuteCommand(nullptr, command, false, false, 0, 0, &textResult, nullptr, TpmLog::Get());

    if (status == MMI_OK)
    {
        commandOutput = (nullptr != textResult) ? std::string(textResult) : "";
    }

    FREE_MEMORY(textResult);

    return commandOutput;
}

int Tpm::Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes)
{
    int status = MMI_OK;
    std::string data;

    if (0 == std::strcmp(TPM, componentName))
    {
        if (0 == std::strcmp(TPM_STATUS, objectName))
        {
            data = std::to_string(static_cast<int>(GetStatus()));
        }
        else if ((0 == std::strcmp(TPM_VERSION, objectName)) && (this->m_hasCapabilitiesFile))
        {
            std::string tpmProperty;
            std::string version = GetVersionFromCapabilitiesFile();

            if (version.empty())
            {
                if (MMI_OK == (status = Tpm2Utils::GetTpmPropertyFromDeviceFile(objectName, tpmProperty)))
                {
                    std::regex re(g_tpmVersionFromDeviceFile);
                    std::smatch match;
                    if (std::regex_search(tpmProperty, match, re))
                    {
                        // version = match[0].str();
                        OsConfigLogInfo(TpmLog::Get(), "Tpm::Get: version: %s", tpmProperty.c_str());
                    }
                }
            }
            else
            {
                OsConfigLogError(TpmLog::Get(), "Version %d", status);
            }

            data = "\"" + version + "\"";
        }
        else if ((0 == std::strcmp(TPM_MANUFACTURER, objectName)) && (this->m_hasCapabilitiesFile))
        {
            std::string tpmProperty;
            std::string manufacturer = GetManufacturerFromCapabilitiesFile();

            if (manufacturer.empty())
            {
                if (MMI_OK == (status = Tpm2Utils::GetTpmPropertyFromDeviceFile(objectName, tpmProperty)))
                {
                    std::regex re(g_tpmManufacturerFromDeviceFile);
                    std::smatch match;
                    if (std::regex_search(tpmProperty, match, re))
                    {
                        // manufacturer = match[0].str();
                        OsConfigLogInfo(TpmLog::Get(), "Tpm::Get: manufacturer: %s", match[0].str().c_str());
                    }
                }
            }
            else
            {
                OsConfigLogError(TpmLog::Get(), "Manufacturer %d", status);
            }

            data = "\"" + manufacturer + "\"";
        }
        else
        {
            OsConfigLogError(TpmLog::Get(), "Invalid objectName: %s", objectName);
            status = EINVAL;
        }
    }
    else
    {
        OsConfigLogError(TpmLog::Get(), "Invalid component name: %s", componentName);
        status = EINVAL;
    }
    if ((m_maxPayloadSizeBytes > 0) && (data.length() > m_maxPayloadSizeBytes))
    {
        OsConfigLogError(TpmLog::Get(), "Payload size %d exceeds max payload size %d", static_cast<int>(data.size()), m_maxPayloadSizeBytes);
        status = E2BIG;
    }
    else
    {
        OsConfigLogInfo(TpmLog::Get(), "TPM %s: '%s' %d", objectName, data.c_str(), (int)data.length());
        *payload = new (std::nothrow) char[data.length()];
        if (nullptr != *payload)
        {
            std::fill(*payload, *payload + data.length(), 0);
            std::memcpy(*payload, data.c_str(), data.length());
            *payloadSizeBytes = data.length();
        }
        else
        {
            OsConfigLogError(TpmLog::Get(), "Failed to allocate memory for payload");
            status = ENOMEM;
        }
    }

    return status;
}

void Tpm::Trim(std::string& str)
{
    if (!str.empty())
    {
        while (str.find(" ") == 0)
        {
            str.erase(0, 1);
        }

        size_t end = str.size() - 1;
        while (str.rfind(" ") == end)
        {
            str.erase(end, end + 1);
            end--;
        }
    }
}

unsigned char Tpm::Decode(char c)
{
    if (('0' <= c) && (c <= '9'))
    {
        return c - '0';
    }
    if (('a' <= c) && (c <= 'f'))
    {
        return c + 10 - 'a';
    }
    if (('A' <= c) && (c <= 'F'))
    {
        return c + 10 - 'A';
    }

    return (unsigned char)-1;
}

void Tpm::HexToText(std::string& s)
{
    std::string result;
    if ((s.size() % 2) == 0)
    {
        const size_t len = s.size() / 2;
        result.reserve(len);

        for (size_t i = 0; i < len; ++i)
        {
            unsigned char c1 = Decode(s[2 * i]) * 16;
            unsigned char c2 = Decode(s[2 * i + 1]);
            if ((c1 != (unsigned char)-1) && (c2 != (unsigned char)-1))
            {
                result += (c1 + c2);
            }
            else
            {
                result.clear();
                break;
            }
        }
    }

    s = result;
}

Tpm::Status Tpm::GetStatus()
{
    std::string commandOutput = RunCommand(g_getTpmDetected);

    if (commandOutput.empty())
    {
        commandOutput = RunCommand(g_getTpmrmDetected);
    }

    std::regex re(g_tpmDetected);
    std::smatch match;
    return std::regex_search(commandOutput, match, re) ? Tpm::Status::TpmDetected : Tpm::Status::TpmNotDetected;
}

std::string Tpm::GetVersionFromCapabilitiesFile()
{
    std::string version;
    std::string commandOutput = RunCommand(g_getTpmCapabilities);

    if (!commandOutput.empty())
    {
        std::regex re(g_tpmVersionFromCapabilitiesFile);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmProperties(match.suffix().str());
            std::string tpmVersion(tpmProperties.substr(0, tpmProperties.find('\n')));
            Trim(tpmVersion);
            // TODO: clean up this line
            version = tpmVersion;
        }
    }

    return version;
}

std::string Tpm::GetManufacturerFromCapabilitiesFile()
{
    std::string manufacturer;
    std::string commandOutput = RunCommand(g_getTpmCapabilities);
    if (!commandOutput.empty())
    {
        std::regex re(g_tpmManufacturerFromCapabilitiesFile);
        std::smatch match;
        if (std::regex_search(commandOutput, match, re))
        {
            std::string tpmProperties(match.suffix().str());
            std::string tpmManufacturer(tpmProperties.substr(0, tpmProperties.find('\n')));
            HexToText(tpmManufacturer);
            Trim(tpmManufacturer);
            // TODO: clean up this line
            manufacturer = tpmManufacturer;
        }
    }

    return manufacturer;
}