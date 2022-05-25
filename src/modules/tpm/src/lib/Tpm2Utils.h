// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef TPM_UTILS_H
#define TPM_UTILS_H

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ScopeGuard.h>
#include <Tpm.h>

#define INT_MAX 0x7FFFFFF
#define TPM_RESPONSE_MAX_SIZE 4096
#define TPM_COMMUNICATION_ERROR -1
#define TPM_PATH "/dev/tpm0"

static const uint8_t g_getTpmProperties[] =
{
    0x80, 0x01, // TPM_ST_NO_SESSIONS
    0x00, 0x00, 0x00, 0x16, // commandSize
    0x00, 0x00, 0x01, 0x7A, // TPM_CC_GetCapability
    0x00, 0x00, 0x00, 0x06, // TPM_CAP_TPM_PROPERTIES
    0x00, 0x00, 0x01, 0x00, // Property: TPM_PT_FAMILY_INDICATOR
    0x00, 0x00, 0x00, 0x66  // propertyCount (102)
};

class Tpm2Utils
{
public:
    static int UnsignedInt8ToUnsignedInt64(uint8_t* buffer, uint32_t size, uint32_t offset, uint32_t length, uint64_t* output)
    {
        int status = 0;
        uint32_t i = 0;
        uint64_t temp = 0;

        // TODO: fix all these log messages

        if (nullptr == buffer)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBuf is null");
            status = EINVAL;
        }
        else if (nullptr == output)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, output is null");
            status = EINVAL;
        }
        else if (offset >= size)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be greater than dataOffset %u", size, offset);
            status = EINVAL;
        }
        else if (INT_MAX < size)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be less than or equal to %u", size, INT_MAX);
            status = EINVAL;
        }
        else if (0 >= length)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must greater than 0", length);
            status = EINVAL;
        }
        else if (length > (size - offset))
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must be less than or equal to %i", length, size - offset);
            status = EINVAL;
        }
        else if (sizeof(uint64_t) < length)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, input buffer dataLength remaining from dataOffset must be less than %zu", sizeof(uint64_t));
            status = EINVAL;
        }
        else
        {
            *output = 0;
            for (i = 0; i < length; i++)
            {
                temp = temp << 8; // Make space to add the next byte of data from the input buffer
                temp += buffer[offset + i];
            }
            *output = temp;
        }

        return status;
    }

    static int GetTpmPropertyFromDeviceFile(const char* objectName, std::string& tpmProperty)
    {
        int status = MMI_OK;
        int tpm = -1;
        const uint8_t* request = g_getTpmProperties;
        ssize_t requestSize = sizeof(g_getTpmProperties);
        uint8_t* buffer = nullptr;
        ssize_t bytes = 0;
        uint64_t propertyKey = 0;

        if (nullptr == (buffer = (uint8_t*)malloc(TPM_RESPONSE_MAX_SIZE)))
        {
            OsConfigLogError(TpmLog::Get(), "Insufficient buffer space available to allocate %d bytes", TPM_RESPONSE_MAX_SIZE);
            status = ENOMEM;
        }
        else
        {
            memset(buffer, 0xFF, TPM_RESPONSE_MAX_SIZE);

            if (-1 == (tpm = open(TPM_PATH, O_RDWR)))
            {
                OsConfigLogError(TpmLog::Get(), "Failed to open tpm: %s", TPM_PATH);
                status = ENOENT;
            }
            else if ((-1 == (bytes = write(tpm, request, requestSize))) || (bytes != requestSize))
            {
                OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
                status = errno;
            }
            else if (-1 == (bytes = read(tpm, buffer, TPM_RESPONSE_MAX_SIZE)))
            {
                OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
                status = errno;
            }
            else
            {
                for (int n = 0x13; n < (TPM_RESPONSE_MAX_SIZE - 8); n += 8)
                {
                    if (0 != UnsignedInt8ToUnsignedInt64(buffer, TPM_RESPONSE_MAX_SIZE, n, 4, &propertyKey))
                    {
                        OsConfigLogError(TpmLog::Get(), "Error converting TPM property key");
                        break;
                    }

                    if ((0x100 == propertyKey) && (0 == std::strcmp(objectName, TPM_VERSION)))
                    {
                        unsigned char tpmPropertyBuffer[5] = {buffer[n + 4], buffer[n + 5], buffer[n + 6], buffer[n + 7], '\0'};
                        tpmProperty = std::string((char*)tpmPropertyBuffer);
                        break;
                    }
                    else if ((0x100 + 5 == propertyKey) && (0 == std::strcmp(objectName, TPM_MANUFACTURER)))
                    {
                        unsigned char tpmPropertyBuffer[5] = {buffer[n + 4], buffer[n + 5], buffer[n + 6], buffer[n + 7], '\0'};
                        tpmProperty = std::string((char*)tpmPropertyBuffer);
                        break;
                    }
                }
            }

            if (tpm != -1)
            {
                close(tpm);
            }

            FREE_MEMORY(buffer);
        }

        return status;
    }
};

#endif // TPM_UTILS_H