#pragma once
//------------------------------------------------------------------------------
/**
    @file component.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace Game
{

typedef MemDb::AttributeId ComponentId;

#define DECLARE_COMPONENT public:\
static Game::ComponentId ID() { return id; }\
private:\
    friend class MemDb::TypeRegistry;\
    static Game::ComponentId id;\
public:

#define DEFINE_COMPONENT(TYPE) Game::ComponentId TYPE::id = Game::ComponentId::Invalid();

} // namespace Game
