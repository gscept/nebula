#pragma once
//------------------------------------------------------------------------------
/**
	Attributes for the transform component.

	(C) 2018 Individual contributors, see AUTHORS file
*/
#include "game/attr/attrid.h"


//------------------------------------------------------------------------------
namespace Attr
{
	DeclareMatrix44(LocalTransform, 'TRLT', Attr::ReadWrite);
	DeclareMatrix44(WorldTransform, 'TRWT', Attr::ReadWrite);
	DeclareInt(Parent, 'TRPT', Attr::ReadOnly);
	DeclareInt(FirstChild, 'TRFC', Attr::ReadWrite);
	DeclareInt(NextSibling, 'TRNS', Attr::ReadOnly);
	DeclareInt(PreviousSibling, 'TRPS', Attr::ReadOnly);
};
//------------------------------------------------------------------------------