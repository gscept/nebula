//------------------------------------------------------------------------------
//  debugpagehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "debug/debugpagehandler.h"
#include "http/html/htmlpagewriter.h"
#include "http/svg/svglinechartwriter.h"
#include "debug/debugserver.h"
#include "debug/debugtimer.h"
#include "debug/debugcounter.h"
#include "util/variant.h"

namespace Debug
{
__ImplementClass(Debug::DebugPageHandler, 'DBPH', Http::HttpRequestHandler);

using namespace Http;
using namespace Util;
using namespace Timing;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
DebugPageHandler::DebugPageHandler()
{
    this->SetName("Profiling");
    this->SetDesc("show profiling (debug) subsystem information");
    this->SetRootLocation("profiling");
}

//------------------------------------------------------------------------------
/**
*/
void
DebugPageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // first check if a command has been defined in the URI
    Dictionary<String,String> query = request->GetURI().ParseQuery();
    if (query.Contains("timer"))
    {
        this->HandleTimerRequest(query["timer"], request);
        return;
    }
    else if (query.Contains("counter"))
    {
        this->HandleCounterRequest(query["counter"], request);
        return;
    }
    else if (query.Contains("timerChart"))
    {
        this->HandleTimerChartRequest(query["timerChart"], request);
        return;
    }
    else if (query.Contains("counterChart"))
    {
        this->HandleCounterChartRequest(query["counterChart"], request);
        return;
    }
    else if (query.Contains("TimerTableSort"))
    {
        this->HandleTimerTableSortRequest(query["TimerTableSort"], query["TimerGroup"], request);
    }
    else if (query.Contains("CounterTableSort"))
    {
        this->HandleCounterTableSortRequest(query["CounterTableSort"], query["CounterGroup"], request);
    }

    // no command, display root page
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Debug Subsystem Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Profiling Subsystem");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");

        // write out timers
        htmlWriter->Element(HtmlElement::Heading2, "Profiling Timers (ms)");

        // iterate through all debug timers and put them in buckets based on group
        const Array<Ptr<DebugTimer>>& debugTimers = DebugServer::Instance()->GetDebugTimers();
        Dictionary<StringAtom, Array<Ptr<DebugTimer>>> groupedTimers;
        IndexT grp;
        for (grp = 0; grp < debugTimers.Size(); ++grp)
        {
            const Ptr<DebugTimer>& timer = debugTimers[grp];
            const Util::StringAtom& group = timer->GetGroup();
            if (!groupedTimers.Contains(group)) 
            {
                groupedTimers.Add(group, Array<Ptr<DebugTimer>>());
            }
            groupedTimers[group].Append(timer);
        }

        for (grp = 0; grp < groupedTimers.Size(); ++grp)
        {
            // get timers and group name
            Array<Ptr<DebugTimer>> debugTimers = groupedTimers.ValueAtIndex(grp);
            const Util::String& group = groupedTimers.KeyAtIndex(grp).AsString();
            Util::String webfriendlyGroup = String::Sprintf("%d", grp);

            // display debug timers
            htmlWriter->Element(HtmlElement::Heading3, String::Sprintf("Debug Timers: %s", group.AsCharPtr()));
            htmlWriter->AddAttr("border", "1");
            htmlWriter->AddAttr("rules", "cols");
            htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?TimerTableSort=Name&TimerGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Name");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?TimerTableSort=Min&TimerGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Min");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?TimerTableSort=Max&TimerGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Max");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?TimerTableSort=Avg&TimerGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Avg");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?TimerTableSort=Cur&TimerGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Cur");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->End(HtmlElement::TableRow);

            // get sorting flag
            Util::String sortColumn;
            if (this->timerSortColumns.Contains(webfriendlyGroup))      sortColumn = this->timerSortColumns[webfriendlyGroup];
            else                                                        sortColumn = "Name";

            // copy to dictionary for sort
            Dictionary<Variant, Ptr<DebugTimer>> sortedTimer;
            sortedTimer.BeginBulkAdd();
            IndexT idx;
            for (idx = 0; idx < debugTimers.Size(); ++idx)
            {
                Array<Time> history = debugTimers[idx]->GetHistory();
                Time minTime, maxTime, avgTime, curTime;
                this->ComputeMinMaxAvgTimes(history, minTime, maxTime, avgTime, curTime);
                if (sortColumn == "Name")
                {
                    sortedTimer.Add(debugTimers[idx]->GetName().Value(), debugTimers[idx]);
                }
                else if (sortColumn == "Min")
                {
                    sortedTimer.Add(float(minTime), debugTimers[idx]);
                }
                else if (sortColumn == "Max")
                {
                    sortedTimer.Add(float(maxTime), debugTimers[idx]);
                }
                else if (sortColumn == "Avg")
                {
                    sortedTimer.Add(float(avgTime), debugTimers[idx]);
                }
                else if (sortColumn == "Cur")
                {
                    sortedTimer.Add(float(curTime), debugTimers[idx]);
                }
            }
            sortedTimer.EndBulkAdd();
            debugTimers = sortedTimer.ValuesAsArray();

            IndexT i;
            for (i = 0; i < debugTimers.Size(); i++)
            {
                StringAtom name = debugTimers[i]->GetName();
                Array<Time> history = debugTimers[i]->GetHistory();
                Time minTime, maxTime, avgTime, curTime;
                this->ComputeMinMaxAvgTimes(history, minTime, maxTime, avgTime, curTime);
                htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Begin(HtmlElement::TableData);
                htmlWriter->AddAttr("href", "/profiling?timer=" + name.AsString());
                htmlWriter->Element(HtmlElement::Anchor, name.AsString());
                htmlWriter->End(HtmlElement::TableData);
                htmlWriter->Element(HtmlElement::TableData, String::FromFloat(float(minTime)));
                htmlWriter->Element(HtmlElement::TableData, String::FromFloat(float(maxTime)));
                htmlWriter->Element(HtmlElement::TableData, String::FromFloat(float(avgTime)));
                htmlWriter->Element(HtmlElement::TableData, String::FromFloat(float(curTime)));
                htmlWriter->End(HtmlElement::TableRow);
            }
            htmlWriter->End(HtmlElement::Table);
        }

        // write counters
        htmlWriter->Element(HtmlElement::Heading2, "Profiling Counters");

        // iterate through all debug timers and put them in buckets based on group
        const Array<Ptr<DebugCounter>>& debugCounters = DebugServer::Instance()->GetDebugCounters();
        Dictionary<StringAtom, Array<Ptr<DebugCounter>>> groupedCounters;
        for (grp = 0; grp < debugCounters.Size(); ++grp)
        {
            const Ptr<DebugCounter>& counter = debugCounters[grp];
            const Util::StringAtom& group = counter->GetGroup();
            if (!groupedCounters.Contains(group))
            {
                groupedCounters.Add(group, Array<Ptr<DebugCounter>>());
            }
            groupedCounters[group].Append(counter);
        }

        for (grp = 0; grp < groupedCounters.Size(); ++grp)
        {
            // get counters and group name
            Array<Ptr<DebugCounter>> debugCounters = groupedCounters.ValueAtIndex(grp);
            const Util::String& group = groupedCounters.KeyAtIndex(grp).AsString();
            Util::String webfriendlyGroup = String::Sprintf("%d", grp);

            // display debug counters
            htmlWriter->Element(HtmlElement::Heading3, String::Sprintf("Debug Counters: %s", group.AsCharPtr()));
            htmlWriter->AddAttr("border", "1");
            htmlWriter->AddAttr("rules", "cols");
            htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?CounterTableSort=Name&CounterGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Name");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?CounterTableSort=Min&CounterGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Min");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?CounterTableSort=Max&CounterGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Max");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?CounterTableSort=Avg&CounterGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Avg");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->Begin(HtmlElement::TableData);
            htmlWriter->AddAttr("href", "/profiling?CounterTableSort=Cur&CounterGroup=" + webfriendlyGroup);
            htmlWriter->Element(HtmlElement::Anchor, "Cur");
            htmlWriter->End(HtmlElement::TableData);
            htmlWriter->End(HtmlElement::TableRow);

            // get sorting flag
            Util::String sortColumn;
            if (this->counterSortColumns.Contains(webfriendlyGroup))    sortColumn = this->counterSortColumns[webfriendlyGroup];
            else                                                        sortColumn = "Name";

            // copy to dictionary for sort
            Dictionary<Variant, Ptr<DebugCounter>> sortedCounters;
            sortedCounters.BeginBulkAdd();
            IndexT idx;
            for (idx = 0; idx < debugCounters.Size(); ++idx)
            {
                Array<int> history = debugCounters[idx]->GetHistory();
                int minTime, maxTime, avgTime, curTime;
                this->ComputeMinMaxAvgCounts(history, minTime, maxTime, avgTime, curTime);
                if (sortColumn == "Name")
                {
                    sortedCounters.Add(debugCounters[idx]->GetName().Value(), debugCounters[idx]);
                }
                else if (sortColumn == "Min")
                {
                    sortedCounters.Add(float(minTime), debugCounters[idx]);
                }
                else if (sortColumn == "Max")
                {
                    sortedCounters.Add(float(maxTime), debugCounters[idx]);
                }
                else if (sortColumn == "Avg")
                {
                    sortedCounters.Add(float(avgTime), debugCounters[idx]);
                }
                else if (sortColumn == "Cur")
                {
                    sortedCounters.Add(float(curTime), debugCounters[idx]);
                }
            }
            sortedCounters.EndBulkAdd();
            debugCounters = sortedCounters.ValuesAsArray();

            // iterate through all debug counters
            IndexT i;
            for (i = 0; i < debugCounters.Size(); i++)
            {
                StringAtom name = debugCounters[i]->GetName();
                Array<int> history = debugCounters[i]->GetHistory();
                int minCount, maxCount, avgCount, curCount;
                this->ComputeMinMaxAvgCounts(history, minCount, maxCount, avgCount, curCount);
                htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Begin(HtmlElement::TableData);
                htmlWriter->AddAttr("href", "/profiling?counter=" + name.AsString());
                htmlWriter->Element(HtmlElement::Anchor, name.Value());
                htmlWriter->End(HtmlElement::TableData);
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(minCount));
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(maxCount));
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(avgCount));
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(curCount));
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

//------------------------------------------------------------------------------
/**
    Gets the min/max/avg time from an array of Time samples.
*/
void
DebugPageHandler::ComputeMinMaxAvgTimes(const Array<Time>& times, Time& outMin, Time& outMax, Time& outAvg, Timing::Time& outCur) const
{
    if (times.Size() > 0)
    {
        outMin = 10000000.0f;
        outMax = -10000000.0f;
        outAvg = 0.0;
        outCur = times.Back();
        IndexT i;
        for (i = 0; i < times.Size(); i++)
        {
            outMin = Math::min(float(outMin), float(times[i]));
            outMax = Math::max(float(outMax), float(times[i]));
            outAvg += times[i];
        }
        outAvg /= times.Size();
    }
    else
    {
        outMin = 0.0;
        outMax = 0.0;
        outAvg = 0.0;
        outCur = 0.0;
    }
}

//------------------------------------------------------------------------------
/**
    Gets the min/max/avg counter values from an array of counter samples.
*/
void
DebugPageHandler::ComputeMinMaxAvgCounts(const Array<int>& counterValues, int& outMin, int& outMax, int& outAvg, int& outCur) const
{
    if (counterValues.Size() > 0)
    {
        outMin = (1<<30);
        outMax = -(1<<30);
        outAvg = 0;
        outCur = counterValues.Back();
        IndexT i;
        for (i = 0; i < counterValues.Size(); i++)
        {
            outMin = Math::min(outMin, counterValues[i]);
            outMax = Math::max(outMax, counterValues[i]);
            outAvg += counterValues[i];
        }
        outAvg /= counterValues.Size();
    }
    else
    {
        outMin = 0;
        outMax = 0;
        outAvg = 0;
        outCur = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Handles a HTTP request for a specific debug timer.
*/
void
DebugPageHandler::HandleTimerRequest(const String& timerName, const Ptr<HttpRequest>& request)
{
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Debug Timer Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Timer: " + timerName);
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/profiling");
        htmlWriter->Element(HtmlElement::Anchor, "Profiling Subsystem Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("data", "/profiling?timerChart=" + timerName);
        htmlWriter->Element(HtmlElement::Object, timerName);    
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

//------------------------------------------------------------------------------
/**
    Handles a HTTP request for a specific debug counter.
*/
void
DebugPageHandler::HandleCounterRequest(const String& counterName, const Ptr<HttpRequest>& request)
{
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Debug Counter Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Counter: " + counterName);
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/profiling");
        htmlWriter->Element(HtmlElement::Anchor, "Profiling Subsystem Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("data", "/profiling?counterChart=" + counterName);
        htmlWriter->Element(HtmlElement::Object, counterName);    
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

//------------------------------------------------------------------------------
/**
    Writes an SVG chart for a debug timer.
*/
void
DebugPageHandler::HandleTimerChartRequest(const String& timerName, const Ptr<HttpRequest>& request)
{
    // setup a SVG line chart writer
    Ptr<DebugTimer> debugTimer = DebugServer::Instance()->GetDebugTimerByName(timerName);
    if (debugTimer.isvalid())
    {
        Ptr<SvgLineChartWriter> writer = SvgLineChartWriter::Create();
        writer->SetStream(request->GetResponseContentStream());
        writer->SetCanvasDimensions(1024, 256);
        if (writer->Open())
        {
            // get min/max/avg times, convert time to float array
            Array<Time> timeArray = debugTimer->GetHistory();
            Time minTime, maxTime, avgTime, curTime;
            this->ComputeMinMaxAvgTimes(timeArray, minTime, maxTime, avgTime, curTime);
            Array<float> floatArray(timeArray.Size(), 0);
            IndexT i;
            for (i = 0; i < timeArray.Size(); i++)
            {
                floatArray.Append(float(timeArray[i]));
            }

            // setup the svg chart writer and draw the chart
            writer->SetupXAxis("frames", "frames", -int(floatArray.Size()), 0);
            writer->SetupYAxis("samples", "samples", 0.0f, float(maxTime));
            writer->AddTrack("samples", "red", floatArray);
            writer->Draw();
            writer->Close();
        }
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::NotFound);
    }
}

//------------------------------------------------------------------------------
/**
    Writes an SVG chart for a debug counter.
*/
void
DebugPageHandler::HandleCounterChartRequest(const String& counterName, const Ptr<HttpRequest>& request)
{
    // setup a SVG line chart writer
    Ptr<DebugCounter> debugCounter = DebugServer::Instance()->GetDebugCounterByName(counterName);
    if (debugCounter.isvalid())
    {
        Ptr<SvgLineChartWriter> writer = SvgLineChartWriter::Create();
        writer->SetStream(request->GetResponseContentStream());
        writer->SetCanvasDimensions(1024, 256);
        if (writer->Open())
        {
            // get min/max/avg values, convert int to float array
            // FIXME: SvgLineChartWriter should also accept integer tracks!
            Array<int> intArray = debugCounter->GetHistory();
            if (intArray.Size() > 0)
            {        
                int minVal, maxVal, avgVal, curVal;
                this->ComputeMinMaxAvgCounts(intArray, minVal, maxVal, avgVal, curVal);

                // for maxVal == minVal set valid maxVal
                if (minVal == maxVal) maxVal = minVal + 1;

                Array<float> floatArray(intArray.Size(), 0);
                IndexT i;
                for (i = 0; i < intArray.Size(); i++)
                {
                    floatArray.Append(float(intArray[i]));
                }

                // setup the svg chart writer and draw the chart
                writer->SetupXAxis("frames", "frames", -int(floatArray.Size()), 0);
                writer->SetupYAxis("samples", "samples", 0.0f, float(maxVal));
                writer->AddTrack("samples", "red", floatArray);
                writer->Draw(); 
            }
            writer->Close();
        }
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::NotFound);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
DebugPageHandler::HandleTimerTableSortRequest(const Util::String& columnName, const Util::String& tableName, const Ptr<Http::HttpRequest>& request)
{
    if (this->timerSortColumns.Contains(tableName))     this->timerSortColumns[tableName] = columnName;
    else                                                this->timerSortColumns.Add(tableName, columnName);
    request->SetStatus(HttpStatus::OK);
}

//------------------------------------------------------------------------------
/**
*/
void
DebugPageHandler::HandleCounterTableSortRequest(const Util::String& columnName, const Util::String& tableName, const Ptr<Http::HttpRequest>& request)
{
    if (this->counterSortColumns.Contains(tableName))   this->counterSortColumns[tableName] = columnName;
    else                                                this->counterSortColumns.Add(tableName, columnName);
    request->SetStatus(HttpStatus::OK);
}

} // namespace Debug
