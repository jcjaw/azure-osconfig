// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <rapidjson/document.h>

#include <Mmi.h>

namespace mmi
{
    // JSON Serializable property types
    class GenericProperty
    {

    };

    class Object
    {

    private:
        std::map<std::string, GenericProperty> m_properties;
    };

    class Component
    {

    private:
        std::map<std::string, Object> m_objects;
    };

    class Module
    {
    public:
        // struct Info
        // {
        //     std::string name;
        //     ...
        // };

        Module(unsigned int maxPayloadSizeBytes);

        // virtual int GetInfo(const char* clientName, MMI_JSON_STRING* payload, int* payloadSizeBytes);
        int Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes);
        int Get(const char* componentName, const char* objectName, MMI_JSON_STRING* payload, int* payloadSizeBytes);

    private:
        std::string m_clientName;
        const unsigned int m_maxPayloadSizeBytes;

        std::map<std::string: Component> m_components;
    };
}

using namespace mmi;

template<class Session>
Module::Module(const char* clientName, unsigned int maxPayloadSizeBytes) :
    m_clientName(clientName),
    m_maxPayloadSizeBytes(maxPayloadSizeBytes) {}

template<class Session>
Module::~Module() {}

template<class Session>
int Module::Set(const char* componentName, const char* objectName, const MMI_JSON_STRING payload, const int payloadSizeBytes)
{

}
