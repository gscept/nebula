//------------------------------------------------------------------------------
//  NavigationPageHandler.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "navigationpagehandler.h"
#include "http/html/htmlpagewriter.h"
#include "http/svg/svglinechartwriter.h"
#include "debug/debugserver.h"
#include "navigationnavigationserver.h"
#include "game/entity.h"
#include "../crowdmanager.h"
#include "DetourCrowd.h"
#include "managers/entitymanager.h"

namespace Navigation
{
__ImplementClass(Navigation::NavigationPageHandler, 'NVPH', Debug::DebugPageHandler);

using namespace Http;
using namespace Util;
using namespace IO;
using namespace Debug;
using namespace Navigation;

//------------------------------------------------------------------------------
/**
*/
NavigationPageHandler::NavigationPageHandler()
{
    this->SetName("Navigation");
    this->SetDesc("show navigation debug information");
    this->SetRootLocation("navigation");
}



//------------------------------------------------------------------------------
/**
*/
static Util::String 
CrowsAgentStateToString(CrowdAgentState i)
{
    switch(i)
    {
        case DT_CROWDAGENT_STATE_INVALID:
            return "Invalid";
        case DT_CROWDAGENT_STATE_WALKING:
            return "Walking";
        case DT_CROWDAGENT_STATE_OFFMESH:
            return "Offmesh";
    }
    return "";
}


//------------------------------------------------------------------------------
/**
*/
static Util::String 
MoveRequestStateToString(MoveRequestState i)
{
    switch(i)
    {
    case DT_CROWDAGENT_TARGET_NONE:
        return "None";
    case DT_CROWDAGENT_TARGET_FAILED:
        return "Failed";
    case DT_CROWDAGENT_TARGET_VALID:
        return "Valid";
    case DT_CROWDAGENT_TARGET_REQUESTING:
        return "Requesting";
    case DT_CROWDAGENT_TARGET_WAITING_FOR_QUEUE:
        return "Waiting for Queue";
    case DT_CROWDAGENT_TARGET_WAITING_FOR_PATH:
        return "Waiting for Path";
    case DT_CROWDAGENT_TARGET_VELOCITY:
        return "Target Velocity";    
    }
    return "";
}


//------------------------------------------------------------------------------
/**
*/
void
NavigationPageHandler::HandleRequest(const Ptr<Http::HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("NebulaT Navigation Info");
    if (htmlWriter->Open())
    {
        String str;
        htmlWriter->Element(HtmlElement::Heading1, "Navigation");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");     

        Dictionary<String,String> query = request->GetURI().ParseQuery();

        if(query.Contains("page"))
        {
            const Util::String & q = query["page"];
            if(q == "server")
            {
                htmlWriter->LineBreak();
                htmlWriter->AddAttr("href", "/navigation");
                htmlWriter->Element(HtmlElement::Anchor, "Navigation Home");

                htmlWriter->Element(HtmlElement::Heading3, "Registered Navmeshes");
                htmlWriter->LineBreak();

                Util::Array<Util::String> meshes = NavigationServer::Instance()->GetNavMeshes();

                htmlWriter->AddAttr("border", "1");
                htmlWriter->AddAttr("rules", "all");

                // begin write table
                htmlWriter->Begin(HtmlElement::Table);

                htmlWriter->AddAttr("bgcolor", "lightsteelblue");
                htmlWriter->Begin(HtmlElement::TableRow);

                htmlWriter->Element(HtmlElement::TableData, "Navmesh");                
                htmlWriter->Element(HtmlElement::TableData, "Selected");
                htmlWriter->End(HtmlElement::TableRow);

                Util::String selected = NavigationServer::Instance()->GetCurrentNavmesh();
                for(IndexT i = 0; i<meshes.Size() ; i++)
                {
                    htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData,meshes[i]);
                    htmlWriter->Element(HtmlElement::TableData,meshes[i] == selected ? "True":"False");                    
                    htmlWriter->End(HtmlElement::TableRow);
                }
                htmlWriter->End(HtmlElement::Table);    
            }
            else if(q == "crowd")
            {
                htmlWriter->LineBreak();
                htmlWriter->AddAttr("href", "/navigation");
                htmlWriter->Element(HtmlElement::Anchor, "Navigation Home");
                if(query.Contains("agents"))
                {
                    int crowdid = query["agents"].AsInt();
                    htmlWriter->Element(HtmlElement::Heading3, "Crowd Agents");

                    dtCrowdAgent ** ags = new dtCrowdAgent *[CrowdManager::Instance()->GetMaxCrowdAgents()];

                    int actives = CrowdManager::Instance()->crowds[crowdid]->crowd->getActiveAgents(ags,CrowdManager::Instance()->GetMaxCrowdAgents());
                                        

                    htmlWriter->AddAttr("border", "1");
                    htmlWriter->AddAttr("rules", "all");

                    // begin write table
                    htmlWriter->Begin(HtmlElement::Table);

                    htmlWriter->AddAttr("bgcolor", "lightsteelblue");

                    htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, "Entity");                
                    htmlWriter->Element(HtmlElement::TableData, "Radius");
                    htmlWriter->Element(HtmlElement::TableData, "Height");
                    htmlWriter->Element(HtmlElement::TableData, "Max Acceleration");
                    htmlWriter->Element(HtmlElement::TableData, "Max Speed");
                    htmlWriter->Element(HtmlElement::TableData, "Separation Weight");
                    htmlWriter->Element(HtmlElement::TableData, "CollisionQuery Range");
                    htmlWriter->Element(HtmlElement::TableData, "Crowd State");
                    htmlWriter->Element(HtmlElement::TableData, "Move Request State");
                    htmlWriter->Element(HtmlElement::TableData, "Position");
                    htmlWriter->Element(HtmlElement::TableData, "Target Position");
                    htmlWriter->Element(HtmlElement::TableData, "Desired Velocity");
                    htmlWriter->Element(HtmlElement::TableData, "Velocity");                                       
                    htmlWriter->End(HtmlElement::TableRow);

                    for(int i = 0 ; i < actives ; i++)
                    {
                        dtCrowdAgent * ag = ags[i];
                        dtCrowdAgentParams ap = ag->params;
                        CrowdManager::agentData* ad = (CrowdManager::agentData*)(ag->params.userData);

                        Util::String frmt;
                        htmlWriter->Begin(HtmlElement::TableRow);
						htmlWriter->Begin(HtmlElement::TableData);
						Util::String category = BaseGameFeature::EntityManager::Instance()->GetEntityByUniqueId(ad->eId)->GetCategory();
						htmlWriter->AddAttr("href", "/objectinspector?ls=" + category + "&id=" + Util::String::FromInt(ad->eId));
						htmlWriter->Element(HtmlElement::Anchor, Util::String::FromInt(ad->eId));
                        htmlWriter->End(HtmlElement::TableData);
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.radius));
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.height));
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.maxAcceleration));
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.maxSpeed));
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.separationWeight));
                        htmlWriter->Element(HtmlElement::TableData, Util::String::FromFloat(ap.collisionQueryRange));
                        htmlWriter->Element(HtmlElement::TableData, CrowsAgentStateToString((CrowdAgentState)ag->state));
                        htmlWriter->Element(HtmlElement::TableData, MoveRequestStateToString((MoveRequestState)ag->targetState));

                        frmt.Format("%f,%f,%f",ag->npos[0],ag->npos[1],ag->npos[2]);
                        htmlWriter->Element(HtmlElement::TableData, frmt);
                        frmt.Format("%f,%f,%f",ag->targetPos[0],ag->targetPos[1],ag->targetPos[2]);
                        htmlWriter->Element(HtmlElement::TableData, frmt);
                        frmt.Format("%f,%f,%f",ag->dvel[0],ag->dvel[1],ag->dvel[2]);
                        htmlWriter->Element(HtmlElement::TableData, frmt);
                        frmt.Format("%f,%f,%f",ag->vel[0],ag->vel[1],ag->vel[2]);
                        htmlWriter->Element(HtmlElement::TableData, frmt);                        

                        htmlWriter->End(HtmlElement::TableRow);

                    }


                    htmlWriter->End(HtmlElement::Table);  
                    htmlWriter->Close();
                    request->SetStatus(HttpStatus::OK);  
                    return;

                }

                htmlWriter->Element(HtmlElement::Heading3, "Crowds");

                htmlWriter->AddAttr("border", "1");
                htmlWriter->AddAttr("rules", "all");

                // begin write table
                htmlWriter->Begin(HtmlElement::Table);

                htmlWriter->AddAttr("bgcolor", "lightsteelblue");
                htmlWriter->Begin(HtmlElement::TableRow);

                htmlWriter->Element(HtmlElement::TableData, "Crowd");                
                htmlWriter->Element(HtmlElement::TableData, "Navmesh");
                htmlWriter->Element(HtmlElement::TableData, "Agent Count");
                htmlWriter->End(HtmlElement::TableRow);

                dtCrowdAgent ** ags = new dtCrowdAgent *[CrowdManager::Instance()->GetMaxCrowdAgents()];

                const Util::Array<CrowdManager::crowdData*> & crowds = CrowdManager::Instance()->crowds;
                for(IndexT i = 0 ; i < crowds.Size() ; i++)
                {
                    htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Begin(HtmlElement::TableData);
                    htmlWriter->AddAttr("href", "/navigation?page=crowd&agents=" + Util::String::FromInt(i));
                    htmlWriter->Element(HtmlElement::Anchor, Util::String::FromInt(i));
                    htmlWriter->End(HtmlElement::TableData);
                    htmlWriter->Element(HtmlElement::TableData, crowds[i]->navMesh);
                    int active = crowds[i]->crowd->getActiveAgents(ags,CrowdManager::Instance()->GetMaxCrowdAgents());
                    htmlWriter->Element(HtmlElement::TableData, Util::String::FromInt(active));
                    htmlWriter->End(HtmlElement::TableRow);
                }
                delete[] ags;
                htmlWriter->End(HtmlElement::Table);                  
            }
            htmlWriter->Close();
            request->SetStatus(HttpStatus::OK);  
            return;
        }        

        // no commands, view objectinspectors mainpage
        htmlWriter->Element(HtmlElement::Heading3, "Available Subsystems");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/navigation?page=server");
        htmlWriter->Element(HtmlElement::Anchor, "Navigation Server");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/navigation?page=crowd");
        htmlWriter->Element(HtmlElement::Anchor, "Crowd System");



        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);        
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Navigation

