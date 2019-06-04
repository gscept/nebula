//------------------------------------------------------------------------------
//  imguigraph.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "foundation/stdneb.h"
#include "imguigraph.h"
#include "imgui.h"


namespace Dynui
{


//------------------------------------------------------------------------------
/**
*/
Graph::Graph(const Util::String & _name, SizeT s) : buffer(s), name(_name), _min(0.0f), _max(1000.0f), average(0.0f), scroll(false)
{
    this->frame_max = -FLT_MAX;
    this->frame_min = FLT_MAX;
    Memory::Fill(this->buffer.GetBuffer(), s * sizeof(float), 0);
}


//------------------------------------------------------------------------------
/**
*/
void
Graph::Draw()
{
    header.Format("%s, average: %f", this->name.AsCharPtr(), this->average);
    this->averageSum = 0.0f;
        
    ImGui::PushID(static_cast<void*>(this));
    ImGui::Checkbox("Scroll", &this->scroll);
    ImGui::SameLine();
    ImGui::Text(Util::String::Sprintf("Min/Max: %f, %f", this->_min, this->_max).AsCharPtr());
    ImGui::SameLine();
    if (ImGui::Button("Reset Min/Max"))
    {
        this->frame_max = -FLT_MAX;
        this->frame_min = FLT_MAX;
    }
    ImVec2 region = ImGui::GetContentRegionAvail();
    ImGui::PlotLines("##plot", Graph::ValueGetter, (void*)this, this->buffer.Size(), 0, this->header.AsCharPtr(), 0.8f * this->_min, 1.2f * this->_max, region);
    ImGui::PopID();
    
    this->_max = this->frame_max;
    this->_min = this->frame_min;
    this->average = this->averageSum / (float)this->buffer.Size();    

}


}