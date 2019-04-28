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
				"transform": [1.0, 0.0, 2.3422, ...]
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
#include "entityloaderbase.h"

//------------------------------------------------------------------------------
namespace BaseGameFeature
{
class JsonEntityLoader : public EntityLoaderBase
{
	__DeclareClass(JsonEntityLoader);
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
