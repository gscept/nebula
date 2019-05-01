//------------------------------------------------------------------------------
//  gamepagehandler.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "basegamefeature/debug/gamepagehandler.h"
#include "http/html/htmlpagewriter.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/componentmanager.h"

namespace Debug
{
__ImplementClass(Debug::GamePageHandler, 'DbGH', Http::HttpRequestHandler);

using namespace IO;
using namespace Http;
using namespace Util;
using namespace Game;

//------------------------------------------------------------------------------
/**
*/
GamePageHandler::GamePageHandler()
{
    this->SetName("Game System");
    this->SetDesc("display game system information");
    this->SetRootLocation("game");
}

//------------------------------------------------------------------------------
/**
*/
void
GamePageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

	// first check if a command has been defined in the URI
	Dictionary<String, String> query = request->GetURI().ParseQuery();
	if (query.Contains("component"))
	{
		this->InspectComponent(query["component"], request);
		return;
	}

	Ptr<EntityManager> entityManager = Game::EntityManager::Instance();
	Ptr<ComponentManager> componentManager = Game::ComponentManager::Instance();
	
    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Game Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Nebula Game System");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");

		if (!entityManager.isvalid() || !componentManager.isvalid())
		{
			htmlWriter->LineBreak();
			htmlWriter->LineBreak();
			htmlWriter->Text("Game system not available.");
		}
		else
		{
			htmlWriter->Element(HtmlElement::Heading3, "Entity System Info");
			htmlWriter->Begin(HtmlElement::Table);
			htmlWriter->Begin(HtmlElement::TableRow);
			htmlWriter->Element(HtmlElement::TableData, "Entity Count:");
			htmlWriter->Element(HtmlElement::TableData, String::FromUInt(entityManager->GetNumEntities()));
			htmlWriter->End(HtmlElement::TableRow);
			htmlWriter->End(HtmlElement::Table);

			htmlWriter->Element(HtmlElement::Heading3, "Registered Components");
			htmlWriter->AddAttr("border", "1");
			htmlWriter->AddAttr("rules", "cols");
			htmlWriter->Begin(HtmlElement::Table);
			htmlWriter->AddAttr("bgcolor", "lightsteelblue");
			htmlWriter->Begin(HtmlElement::TableRow);
			htmlWriter->Element(HtmlElement::TableHeader, "Component");
			htmlWriter->Element(HtmlElement::TableHeader, "Registered entities");
			htmlWriter->Element(HtmlElement::TableHeader, "Attribute count");
			htmlWriter->End(HtmlElement::TableRow);
			SizeT numComponents = componentManager->GetNumComponents();
			IndexT componentIndex;
			for (componentIndex = 0; componentIndex < numComponents; componentIndex++)
			{
				ComponentInterface* component = componentManager->GetComponentAtIndex(componentIndex);
				htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Begin(HtmlElement::TableData);
				htmlWriter->AddAttr("href", "/game?component=" + component->GetIdentifier().AsString());
				htmlWriter->Element(HtmlElement::Anchor, component->GetName().AsString());
				htmlWriter->End(HtmlElement::TableData);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(component->NumRegistered()));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(component->GetAttributes().Size()));
				htmlWriter->End(HtmlElement::TableRow);
			}
			htmlWriter->End(HtmlElement::Table);
		}
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

void GamePageHandler::InspectComponent(const Util::FourCC& fourcc, const Ptr<Http::HttpRequest>& request)
{
	Ptr<EntityManager> entityManager = Game::EntityManager::Instance();
	Ptr<ComponentManager> componentManager = Game::ComponentManager::Instance();

	// configure a HTML page writer
	Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
	htmlWriter->SetStream(request->GetResponseContentStream());
	htmlWriter->SetTitle("Nebula Game Info");
	if (htmlWriter->Open())
	{
		htmlWriter->Element(HtmlElement::Heading1, "Nebula Component System");
		htmlWriter->AddAttr("href", "/index.html");
		htmlWriter->Element(HtmlElement::Anchor, "Home");
		htmlWriter->LineBreak();
		htmlWriter->AddAttr("href", "/game");
		htmlWriter->Element(HtmlElement::Anchor, "Game Subsystem Home");

		if (!entityManager.isvalid() || !componentManager.isvalid())
		{
			htmlWriter->LineBreak();
			htmlWriter->LineBreak();
			htmlWriter->Text("Game system not available.");
		}
		else
		{
			ComponentInterface* component = componentManager->GetComponentByFourCC(fourcc);
			if (component == nullptr)
			{
				htmlWriter->Text("No component found with provided FourCC.");
			}
			else
			{
				SizeT numInstances = component->NumRegistered();
				htmlWriter->Element(HtmlElement::Heading3, component->GetName().AsString());
				htmlWriter->Begin(HtmlElement::Table);
				htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableData, "Registered Entities:");
				htmlWriter->Element(HtmlElement::TableData, String::FromUInt(numInstances));
				htmlWriter->End(HtmlElement::TableRow);
				htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableData, "Attribute Count:");
				htmlWriter->Element(HtmlElement::TableData, String::FromUInt(component->GetAttributes().Size()));
				htmlWriter->End(HtmlElement::TableRow);
				htmlWriter->End(HtmlElement::Table);

				htmlWriter->Element(HtmlElement::Heading3, "Instances");
				htmlWriter->AddAttr("border", "1");
				htmlWriter->AddAttr("rules", "all");
				htmlWriter->Begin(HtmlElement::Table);
				htmlWriter->AddAttr("bgcolor", "lightsteelblue");
				htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableHeader, "Instance Id");
				htmlWriter->Element(HtmlElement::TableHeader, "Entity Id");
				const auto& attributes = component->GetAttributes();
				for (SizeT i = 1; i < attributes.Size(); i++)
				{
					htmlWriter->Element(HtmlElement::TableHeader, attributes[i].name);
				}
				htmlWriter->End(HtmlElement::TableRow);
				
				IndexT instance;
				for (instance = 0; instance < numInstances; instance++)
				{
					htmlWriter->AddAttr("align", "left");
					if (instance % 2 == 1)
						htmlWriter->AddAttr("bgcolor", "gainsboro");
					htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(instance));
					htmlWriter->Element(HtmlElement::TableData, String::FromUInt(component->GetOwner(instance).id));
					for (SizeT i = 1; i < attributes.Size(); i++)
					{
						htmlWriter->Begin(HtmlElement::TableHeader);
						String str = "";
						Util::Variant value = component->GetAttributeValue(instance, i);
						switch (attributes[i].type)
						{
						case Attr::ValueType::Matrix44Type:
						{
							Math::matrix44 mat4 = value.GetMatrix44();
							htmlWriter->Text(String::FromFloat4(mat4.getrow0()));
							htmlWriter->LineBreak();
							htmlWriter->Text(String::FromFloat4(mat4.getrow1()));
							htmlWriter->LineBreak();
							htmlWriter->Text(String::FromFloat4(mat4.getrow2()));
							htmlWriter->LineBreak();
							htmlWriter->Text(String::FromFloat4(mat4.getrow3()));
							break;
						}
						case Attr::ValueType::IntType:
							str = String::FromInt(value.GetInt());
							break;
						case Attr::ValueType::UIntType:
						case Attr::ValueType::EntityType:
							if (value.GetUInt() == InvalidIndex)
							{
								str = "-1";
							}
							else
							{
								str = String::FromUInt(value.GetUInt());
							}
							break;
						case Attr::ValueType::Int64Type:
							str = String::FromLongLong(value.GetInt64());
							break;
						case Attr::ValueType::UInt64Type:
							str = String::FromLongLong(value.GetUInt64());
							break;
						case Attr::ValueType::ByteType:
							str = String::FromByte(value.GetByte());
							break;
						case Attr::ValueType::StringType:
							str = value.GetString();
							break;
						case Attr::ValueType::UShortType:
							str = String::FromUShort(value.GetUShort());
							break;
						case Attr::ValueType::ShortType:
							str = String::FromShort(value.GetShort());
							break;
						case Attr::ValueType::BoolType:
							str = String::FromBool(value.GetBool());
							break;
						case Attr::ValueType::QuaternionType:
							str = String::FromQuaternion(value.GetQuaternion());
							break;
						case Attr::ValueType::Float4Type:
							str = String::FromFloat4(value.GetFloat4());
							break;
						case Attr::ValueType::Float2Type:
							str = String::FromFloat2(value.GetFloat2());
							break;
						case Attr::ValueType::FloatType:
							str = String::FromFloat(value.GetFloat());
							break;
						case Attr::ValueType::DoubleType:
							str = String::FromDouble(value.GetDouble());
							break;
						default:
							str = "";
							break;
						}
						if (!str.IsEmpty())
							htmlWriter->Text(str);
						htmlWriter->End(HtmlElement::TableHeader);
					}
					htmlWriter->End(HtmlElement::TableRow);
				}
				htmlWriter->End(HtmlElement::Table);
			}
		}
		htmlWriter->Close();
		request->SetStatus(HttpStatus::OK);
	}
	else
	{
		request->SetStatus(HttpStatus::InternalServerError);
	}

}

} // namespace Debug
