#include "MimParser.h"

static const std::string g_mimModel = "MimModel";
static const std::string g_mimObject = "mimObject";

pMimObjects MimParser::ParseMim(std::string path)
{
    JSON_Value *root_value;
    root_value = json_parse_file(path.c_str());

    pMimObjects mimObjects = std::make_shared<MimObjects>();

    if (json_value_get_type(root_value) != JSONObject)
    {
        json_value_free(root_value);
        return mimObjects;
    }

    JSON_Object *root_object = json_value_get_object(root_value);
    JSON_Array *components = json_object_get_array(root_object, "contents");
    JSON_Array *jsonMimObjects = nullptr;

    for (size_t i = 0; i < json_array_get_count(components); i++)
    {
        // Components
        root_object = json_array_get_object(components, i);

        std::string componentName = json_object_get_string(root_object, "name");

        jsonMimObjects = json_object_get_array(root_object, "contents");
        for (size_t y = 0; y < json_array_get_count(jsonMimObjects); y++)
        {
            root_object = json_array_get_object(jsonMimObjects, y);

            if (0 == strcmp(json_object_get_string(root_object, "type"), g_mimObject.c_str()))
            {
                MimObject mim = {
                    json_object_get_string(root_object, "name"),
                    json_object_get_string(root_object, "type"),
                    !!json_object_get_boolean(root_object, "desired"),
                    std::make_shared<std::map<std::string, MimField>>()};

                // Get fields
                JSON_Object *schema_object = json_object_get_object(root_object, "schema");
                JSON_Array *jsonFields = json_object_get_array(schema_object, "fields");
                for (size_t z = 0; z < json_array_get_count(jsonFields); z++)
                {
                    JSON_Object *jsonField = json_array_get_object(jsonFields, z);
                    MimField mimField;
                    if (json_object_has_value_of_type(jsonField, "schema", JSONString) ||
                        json_object_has_value_of_type(jsonField, "schema", JSONNumber) ||
                        json_object_has_value_of_type(jsonField, "schema", JSONBoolean))
                    {
                        mimField = {
                            json_object_get_string(jsonField, "name"),
                            json_object_get_string(jsonField, "schema"),
                            "", // subType1
                            "", // subType2
                            std::make_shared<std::vector<std::string>>()};
                    }
                    else if (json_object_has_value_of_type(jsonField, "schema", JSONObject))
                    {
                        // TODO: Turn into recursive function to support embedded objects?

                        JSON_Object *jsonSchema = json_object_get_object(jsonField, "schema");

                        // TODO: Add type check -- need map + array support
                        // Map = mapKey + schema, mapValue + schema
                        // Only for enum type Values
                        if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "enum"))
                        {
                            mimField = {
                                json_object_get_string(jsonField, "name"),
                                json_object_get_string(jsonSchema, "valueSchema"),
                                "", // subType1
                                "", // subType2
                                std::make_shared<std::vector<std::string>>()};

                            // Add supported values
                            if (json_object_has_value_of_type(jsonSchema, "enumValues", JSONArray))
                            {
                                JSON_Array *supportedValues = json_object_get_array(jsonSchema, "enumValues");
                                for (size_t a = 0; a < json_array_get_count(supportedValues); a++)
                                {
                                    JSON_Object *jsonField = json_array_get_object(supportedValues, a);
                                    mimField.allowedValues->push_back(std::to_string(json_object_get_number(jsonField, "enumValue")));
                                }
                            }
                        }
                        // else if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "array"))
                        // {
                        //     // TODO: Add support for array of objects
                        //     mimField = {
                        //         json_object_get_string(jsonField, "name"),
                        //         "array",
                        //         json_object_get_string(jsonField, "elementSchema"), // subType1
                        //         "", // subType2
                        //         std::make_shared<std::vector<std::string>>()};
                        // }
                        else if (0 == strcmp(json_object_get_string(jsonSchema, "type"), "map"))
                        {
                            const char* keySchema = json_object_dotget_string(jsonSchema, "mapKey.schema");
                            const char* valueSchema = json_object_dotget_string(jsonSchema, "mapValue.schema");

                            if (nullptr == keySchema || nullptr == valueSchema)
                            {
                                TestLogError("Missing key or value schema for map field '%s'", json_object_get_string(jsonSchema, "name"));
                            }

                            mimField = {
                                json_object_get_string(jsonField, "name"),
                                "map",
                                keySchema, // subType1
                                valueSchema, // subType2
                                std::make_shared<std::vector<std::string>>()};
                        }
                        else
                        {
                            TestLogError("Invalid type '%s'", json_object_get_string(jsonSchema, "type"));
                        }
                    }
                    (*mim.m_fields)[mimField.name] = mimField;
                }

                std::shared_ptr<std::map<std::string, MimObject>> mimObjectsPtr = nullptr;
                try
                {
                    mimObjectsPtr = mimObjects->at(componentName);
                }
                catch (const std::out_of_range &e)
                {
                    (*mimObjects)[componentName] = std::make_shared<std::map<std::string, MimObject>>();
                    mimObjectsPtr = (*mimObjects)[componentName];
                }

                (*mimObjectsPtr)[mim.m_name] = mim;
            }
        }
    }

    json_value_free(root_value);
    return mimObjects;
}