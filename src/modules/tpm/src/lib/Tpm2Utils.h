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
    static int UnsignedInt8ToUnsignedInt64(uint8_t* inputBuf, uint32_t inputBufSize, uint32_t dataOffset, uint32_t dataLength, uint64_t* output)
    {
        int status = 0;
        uint32_t i = 0;
        uint64_t temp = 0;

        if (nullptr == inputBuf)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBuf is null");
            status = EINVAL;
        }
        else if (nullptr == output)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, output is null");
            status = EINVAL;
        }
        else if (dataOffset >= inputBufSize)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be greater than dataOffset %u", inputBufSize, dataOffset);
            status = EINVAL;
        }
        else if (INT_MAX < inputBufSize)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, inputBufSize %u must be less than or equal to %u", inputBufSize, INT_MAX);
            status = EINVAL;
        }
        else if (0 >= dataLength)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must greater than 0", dataLength);
            status = EINVAL;
        }
        else if (dataLength > (inputBufSize - dataOffset))
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, dataLength %u must be less than or equal to %i", dataLength, inputBufSize - dataOffset);
            status = EINVAL;
        }
        else if (sizeof(uint64_t) < dataLength)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, input buffer dataLength remaining from dataOffset must be less than %zu", sizeof(uint64_t));
            status = EINVAL;
        }
        else
        {
            *output = 0;
            for (i = 0; i < dataLength; i++)
            {
                temp = temp << 8; // Make space to add the next byte of data from the input buffer
                temp += inputBuf[dataOffset + i];
            }
            *output = temp;
        }

        return status;
    }

    static int BufferToString(unsigned char* buffer, std::string& str)
    {
        int status = MMI_OK;
        if (nullptr == buffer)
        {
            if (IsFullLoggingEnabled())
            {
                OsConfigLogError(TpmLog::Get(), "Invalid argument, buf is null");
            }
            status = EINVAL;
        }
        else
        {
            std::ostringstream os;
            os << buffer;
            if (os.good())
            {
                str = os.str();
            }
            else
            {
                if (IsFullLoggingEnabled())
                {
                    OsConfigLogError(TpmLog::Get(), "Error populating std::ostringstream");
                }
                status = ENOMEM;
            }
        }

        return status;
    }

    static std::string GetTpmPropertyFromBuffer(uint8_t* buffer, ssize_t bufSize, const std ::string objectName)
    {
        std::string tpmProperty;
        uint64_t tpmPropertyKey = 0;

        if (nullptr == buffer)
        {
            OsConfigLogError(TpmLog::Get(), "Invalid argument, null buffer");
        }
        else
        {
            for(int n = 0x13; n < (bufSize - 8); n += 8)
            {
                if (MMI_OK != (status = UnsignedInt8ToUnsignedInt64(buf, TPM_RESPONSE_MAX_SIZE, n, 4, &tpmPropertyKey)))
                {
                    break;
                }

                unsigned char nullTerminator = '\0';

                switch(tpmPropertyKey)
                {
                    case 0x100:
                    {
                        if (0 == std::strcmp(objectName, TPM_VERSION))
                        {
                            unsigned char tpmPropertyBuffer[5] = {buf[n + 4], buf[n + 5], buf[n + 6], buf[n + 7], nullTerminator};
                            status = BufferToString(tpmPropertyBuffer, tpmProperty);
                        }
                        break;
                    }
                    case 0x100+5:
                    {
                        if (0 == std::strcmp(objectName, TPM_MANUFACTURER))
                        {
                            unsigned char tpmPropertyBuffer[5] = {buf[n + 4], buf[n + 5], buf[n + 6], buf[n + 7], nullTerminator};
                            status = BufferToString(tpmPropertyBuffer, tpmProperty);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        return tpmProperty;
    }

    static int GetTpmPropertyFromDeviceFile(const char* objectName, std::string& tpmProperty)
    {
        int status = MMI_OK;
        int tpm = -1;
        uint8_t* request = g_getgetTpmProperties;
        ssize_t requestSize = sizeof(g_getTpmProperties);
        uint8_t* response = nullptr
        ssize_t responseSize = TPM_RESPONSE_MAX_SIZE;

        if (nullptr == (response = (uint8_t*)malloc(responseSize)))
        {
            OsConfigLogError(TpmLog::Get(), "Insufficient buffer space available to allocate %d bytes", TPM_RESPONSE_MAX_SIZE);
            status = ENOMEM;
        }
        else
        {
            memset(response, 0xFF, responseSize);

            if (-1 == (tpm = open(TPM_PATH, O_RDWR)))
            {
                OsConfigLogError(TpmLog::Get(), "Failed to open tpm: %s", TPM_PATH);
                status = ENOENT;
            }
            else if ((-1 == (responseSize = write(tpm, request, requestSize))) || (requestSize != responseSize))
            {
                OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
                status = errno;
            }
            else if (-1 == (responseSize = read(tpm, response, TPM_RESPONSE_MAX_SIZE)))
            {
                OsConfigLogError(TpmLog::Get(), "Error reading response from the device");
                status = errno;
            }
            else
            {
                status = GetTpmPropertyFromBuffer(buf, responseSize, objectName, tpmProperty)
            }

            if (tpm != -1)
            {
                close(tpm);
            }
        }

        return status;
    }
};

#endif // TPM_UTILS_H