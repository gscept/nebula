#include "stdneb.h"
#include "entityattr.h"

namespace Attr
{
    __DefineAttribute(Owner, Game::Entity, 'OWNR', Attr::ReadOnly, uint(-1));
}