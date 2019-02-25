#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::JsonEntityLoader
    
    Json type entity loader.

    Entity format is defined as:
    \code{.json}
    {
        "entities": {
            "Util::Guid": {
                "name": String,
                "components": {
                    "ComponentName": {
                        "active": Boolean,
                        "attributes": {
                            "AttributeName": VALUE,
                            "AttributeName2": VALUE
                        }
                    },
                    "ComponentName2": {
                        ...
                    }
                }
            },
            "GUID2": {
                ...
            }
        }
    }
    \endcode

    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "core/refcounted.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{
class JsonEntityLoader : public Core::RefCounted
{
public:
    /// constructor
    JsonEntityLoader();
    /// destructor
    ~JsonEntityLoader();
    /// load entity objects into the level
    virtual bool Load(const Util::String& file);
    /// is loader currently inside Load Function
    static bool IsLoading();

protected:
    static bool insideLoading;
};

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
