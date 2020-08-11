#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::EntitySystemTest

    Tests Game::Database functionality.

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include "game/database/attribute.h"
#include "game/entity.h"
#include "game/messaging/message.h"
#include "game/property.h"

namespace Attr
{
__DeclareAttribute(ObjectTransform, AccessMode::ReadWrite, Math::mat4, 'Ttrs', Math::mat4());
__DeclareAttribute(Health, AccessMode::ReadWrite, int, 'T-hp', int(100));
__DeclareAttribute(Speed, AccessMode::ReadWrite, float, 'Tspd', float(5.0f));
__DeclareAttribute(MoveDirection, AccessMode::ReadWrite, Math::vec4, 'Tdir', Math::vec4(1, 2, 3, 4));
__DeclareAttribute(Damage, AccessMode::ReadWrite, float, 'Tdmg', 5.0f);
__DeclareAttribute(Target, AccessMode::ReadWrite, Game::Entity, 'Ttrg', Game::Entity::Invalid());
}

//------------------------------------------------------------------------------
namespace Test
{

__DeclareMsg(TestMsg, 'tsMs', int, float);

class TestProperty : public Game::Property
{
    __DeclareClass(TestProperty);
public:
    TestProperty()
    {
    }
    ~TestProperty()
    {
    }

    void Init() override;
    void OnBeginFrame() override;

    void SetupExternalAttributes() override;
private:
    void RecieveTestMsg(int i, float f);

    struct Data
    {
        Game::PropertyData<int> health;
        Game::PropertyData<float> speed;
        Game::PropertyData<Math::vec4> moveDirection;
        Game::PropertyData<Math::mat4> transform;
    } data;
};

class EntitySystemTest : public TestCase
{
    __DeclareClass(EntitySystemTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
