//------------------------------------------------------------------------------
//  transformattr.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformattr.h"

namespace Attr
{
	DefineMatrix44WithDefault(LocalTransform, 'TFLT', Attr::ReadWrite, Math::matrix44::identity())
	DefineMatrix44WithDefault(WorldTransform, 'TRWT', Attr::ReadWrite, Math::matrix44::identity());
	DefineInt(Parent, 'TRPT', Attr::ReadOnly);
	DefineInt(FirstChild, 'TRFC', Attr::ReadWrite);
	DefineInt(NextSibling, 'TRNS', Attr::ReadOnly);
	DefineInt(PreviousSibling, 'TRPS', Attr::ReadOnly);
} // namespace Base
