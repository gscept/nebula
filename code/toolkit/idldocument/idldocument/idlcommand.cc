//------------------------------------------------------------------------------
//  idlcommand.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idlcommand.h"

namespace Tools
{
__ImplementClass(Tools::IDLCommand, 'ILCM', Core::RefCounted);

using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
IDLCommand::IDLCommand()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCommand::Parse(XmlReader* reader)
{
    n_assert(0 != reader);
    n_assert(reader->GetCurrentNodeName() == "Command");

    // parse attributes
    this->name = reader->GetString("name");
    this->desc = reader->GetString("desc");
    this->fourcc = reader->GetString("fourcc");

    // parse input args
    if (reader->SetToFirstChild("InArg")) do
    {
        Ptr<IDLArg> arg = IDLArg::Create();
        if (!arg->Parse(reader))
        {
            this->SetError(arg->GetError());
            return false;
        }
        this->inArgs.Append(arg);
    }
    while (reader->SetToNextChild("InArg"));

    // parse output args
    if (reader->SetToFirstChild("OutArg")) do
    {
        Ptr<IDLArg> arg = IDLArg::Create();
        if (!arg->Parse(reader))
        {
            this->SetError(arg->GetError());
            return false;
        }
        this->outArgs.Append(arg);
    }
    while (reader->SetToNextChild("OutArg"));

    // read source code fragment
    bool success = reader->SetToFirstChild("Code");
    n_assert2(success, "<Code> element expected!");
    this->code = reader->GetContent();
    reader->SetToParent();

    return true;
}

//------------------------------------------------------------------------------
/**
*/
String
FromNebulaRefType(const String& type) 
{
    if ("Util::String" == type)                     return "string";
    else if ("Math::float4" == type)                return "float4";
    else if ("Math::vector" == type)                return "float4";
    else if ("Math::matrix44" == type)              return "matrix44";
    else if ("Math::transform44" == type)           return "transform44";
    else if ("bool" == type)                        return "bool";
    else if ("Util::Guid" == type)                  return "guid";
    else if ("int" == type)                         return "int";
    else if ("uint" == type)                        return "uint";
    else if ("float" == type)                       return "float";
    else if ("Util::Array<int>" == type)            return "intArray";
    else if ("Util::Array<float>" == type)          return "floatArray";
    else if ("Util::Array<bool>" == type)           return "boolArray";
    else if ("Util::Array<Util::String>" == type)   return "stringArray";
    else if ("Util::Array<Math::vector>" == type)   return "vectorArray";
    else if ("Util::Array<Math::float4>" == type)   return "float4Array";
    else if ("Util::Array<Math::matrix44>" == type) return "matrix44Array";
    else if ("Util::Array<Util::Guid>" == type)     return "guidArray";
    // hopefully a wrapped type, otherwise output will be crap
    else return type;
    //else
    //{
    //	n_error("Invalid IDL type '%s'!", type.AsCharPtr());
    //	return "";
    //}
}

//------------------------------------------------------------------------------
/**
*/
bool
IDLCommand::WrapMessage(const Ptr<IDLMessage> & msg, const Util::String & mnamespace)
{	
	this->name = msg->GetName();	
	this->fourcc = msg->GetFourCC();
	this->fourcc[0] = '/';
	Ptr<IDLArg> entityIdArg = IDLArg::Create();
	entityIdArg->CreateFromArgs("___entityId","uint","0","");

	this->inArgs.Append(entityIdArg);
	for(int i=0;i<msg->GetInputArgs().Size();i++)
	{
		Ptr<IDLArg> arg = IDLArg::Create();
		arg->CreateFromArgs(msg->GetInputArgs()[i]->GetName(),FromNebulaRefType(msg->GetInputArgs()[i]->GetType()),msg->GetInputArgs()[i]->GetDefaultValue(),msg->GetInputArgs()[i]->GetWrappingType());
		this->inArgs.Append(arg);
	}
	for(int i=0;i<msg->GetOutputArgs().Size();i++)
	{
		Ptr<IDLArg> arg = IDLArg::Create();
		arg->CreateFromArgs(msg->GetOutputArgs()[i]->GetName(),FromNebulaRefType(msg->GetOutputArgs()[i]->GetType()),msg->GetOutputArgs()[i]->GetDefaultValue(),msg->GetOutputArgs()[i]->GetWrappingType());
		this->outArgs.Append(arg);
	}		
	Util::String codeHead;
	codeHead.Format("	Ptr<Game::Entity> entity = BaseGameFeature::EntityManager::Instance()->GetEntityByUniqueId(___entityId);\n"
					"	n_assert2(entity.isvalid(),\"Provided invalid entity id or entity not found!\");\n"
					"	Ptr<%s::%s> msg = %s::%s::Create();\n",mnamespace.AsCharPtr(), this->name.AsCharPtr(),mnamespace.AsCharPtr(),this->name.AsCharPtr());
	for(int i=0;i<msg->GetInputArgs().Size();i++)
	{
		Util::String arg;		
		arg.Format("	msg->Set%s(%s);\n",msg->GetInputArgs()[i]->GetName().AsCharPtr(),msg->GetInputArgs()[i]->GetName().AsCharPtr());
		codeHead.Append(arg);
	}
	Util::String codeFooter = "	entity->SendSync(msg.cast<Messaging::Message>());";
	codeHead.Append(codeFooter);
	if(msg->GetOutputArgs().Size() > 0)
	{
		IDLArg* arg = msg->GetOutputArgs()[0];
		Util::String retvalue;
		retvalue.Format("\n   return msg->Get%s();",arg->GetName().AsCharPtr());
		codeHead.Append(retvalue);
	}
	this->code = codeHead;
	if(!msg->GetDescription().IsEmpty())
	{
		this->desc = msg->GetDescription();
	}
	return true;
}



} // namespace Tools
