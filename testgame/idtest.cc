//------------------------------------------------------------------------------
//  idtest.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "idtest.h"
#include "ids/id.h"

using namespace Game;

namespace Test
{
__ImplementClass(Test::IdTest, 'IDTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
IdTest::Run()
{
    IdSystem id;    
    Id tmp = id.Allocate();
    this->Verify(tmp.Index() == 0);
    this->Verify(id.IsValid(tmp));
    id.Deallocate(tmp);
    this->Verify(!id.IsValid(tmp));
    Id tmp2 = id.Allocate();
    this->Verify(tmp.Index() != tmp2.Index());
    id.Deallocate(tmp2);
    Util::Array<Game::Id> ids;
    uint32_t lastidx = tmp2.Index();
    bool increase = true;
    for (int i = 0; i < 1022; i++)
    {
        Id tmp = id.Allocate();
        increase &= tmp.Index() > lastidx;        
        lastidx = tmp.Index();
        id.Deallocate(tmp);
    }
    this->Verify(increase);
    for (int i = 0; i < 1022; i++)
    {
        Id tmp = id.Allocate();
        increase &= tmp.Index() <1024;        
        id.Deallocate(tmp);
    }
    this->Verify(increase);
}

}