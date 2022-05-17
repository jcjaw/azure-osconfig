// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef MIMPARSER_H
#define MIMPARSER_H

#include <map>

// define a MimObject interface

// TODO: Must make sure reported object is present if defined. Create testrecipe with them.
// TODO: create a high-level testrecipe - capture
// ComponentName	String	Name of the MIM component.
// ObjectName	String	Name of the MIM object.
// Desired	Boolean	True means desired object and false means reported object.
// Payload	String	The JSON payload as escaped JSON. For a desired object this is the desired payload for MmiSet. For a reported object this is the expected reported payload for MmiGet or “<dynamic>” (TBD) meaning dynamic payload that cannot be already known by the test. 
// PayloadSizeBytes	Integer	The size of the payload, in bytes. 
// ExpectedResult	Integer	The expected result (such as MMI_OK).
// WaitSeconds	Integer	The wait time, in seconds, the test must wait after making this test’s MmiSet or MmiGet request, before making the next test request.

class MimParser
{
private:
    /* data */
    // Map of components
    std::map<std::string, std::string> m_components;
    // map of MimObjects + types
    std::map<std::string, std::string> m_types;
public:
    
};

#endif //MIMPARSER_H