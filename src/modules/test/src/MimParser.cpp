#include "MimParser.h"

#include <parson.h>

void MimParser::ParseMim(std::string path)
{
    //TODO: populate m_components with components and populate the components
    // TODO: root should be the MimModel.
    JSON_Value *root_value;
    root_value = json_parse_file(path.c_str());

    json_value_free(root_value);
}