#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::DebugPageHandler
    
    Http request handler for the Debug subsystem.

	Renders profiling counters and timers.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "http/httprequesthandler.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Debug
{
class DebugPageHandler : public Http::HttpRequestHandler
{
    __DeclareClass(DebugPageHandler);
public:
    /// constructor
    DebugPageHandler();
    /// handle a http request
    virtual void HandleRequest(const Ptr<Http::HttpRequest>& request);

protected:
    /// compute the min, max and average time from an array of times
    void ComputeMinMaxAvgTimes(const Util::Array<Timing::Time>& times, Timing::Time& outMin, Timing::Time& outMax, Timing::Time& outAvg, Timing::Time& outCur) const;
    /// compute min, max and average values from an array of counter samples
    void ComputeMinMaxAvgCounts(const Util::Array<int>& counterValues, int& outMin, int& outMax, int& outAvg, int& outCur) const;

private:
    /// handle HTTP request for a debug timer
    void HandleTimerRequest(const Util::String& timerName, const Ptr<Http::HttpRequest>& request);
    /// handle HTTP request for a debug counter
    void HandleCounterRequest(const Util::String& counterName, const Ptr<Http::HttpRequest>& request);
    /// handle HTTP request to render a timer chart
    void HandleTimerChartRequest(const Util::String& timerName, const Ptr<Http::HttpRequest>& request);
    /// handle HTTP request to render a counter char
    void HandleCounterChartRequest(const Util::String& counterName, const Ptr<Http::HttpRequest>& request);
    /// handle HTTP request to sort table
    void HandleTimerTableSortRequest(const Util::String& columnName, const Util::String& tableName, const Ptr<Http::HttpRequest>& request);
	/// handle HTTP request to sort table
	void HandleCounterTableSortRequest(const Util::String& columnName, const Util::String& tableName, const Ptr<Http::HttpRequest>& request);

	Util::Dictionary<Util::String, Util::String> timerSortColumns;
	Util::Dictionary<Util::String, Util::String> counterSortColumns;
};

} // namespace Debug
//------------------------------------------------------------------------------

