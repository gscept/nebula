//------------------------------------------------------------------------------
//  memorypagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "memory/debug/memorypagehandler.h"
#include "memory/heap.h"
#include "http/html/htmlpagewriter.h"
#include "memory/poolarrayallocator.h"

namespace Debug
{
__ImplementClass(Debug::MemoryPageHandler, 'MPGH', Http::HttpRequestHandler);

using namespace IO;
using namespace Http;
using namespace Util;
using namespace Memory;

//------------------------------------------------------------------------------
/**
*/
MemoryPageHandler::MemoryPageHandler()
{
    this->SetName("Memory");
    this->SetDesc("show memory debug information");
    this->SetRootLocation("memory");
}

//------------------------------------------------------------------------------
/**
*/
void
MemoryPageHandler::HandleRequest(const Ptr<HttpRequest>& request) 
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // configure a HTML page writer
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Memory Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Memory");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // if not compiled with NEBULA_MEMORY_STATS, display a message
        #if (NEBULA_MEMORY_STATS == 0)
        htmlWriter->Text("Memory stats not available because NEBULA_MEMORY_STATS was not defined "
                         "when application was compiled. Go to /foundation/core/config.h, change "
                         "NEBULA_MEMORY_STATS to (1) and recompile the application!");
        #else

        htmlWriter->Text("This includes all allocations that go through the Memory subsystem "
                         "functions (this includes the creation of all Nebula objects). Other memory "
                         "allocations such as DirectX resources are NOT included!");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // display overall stats
        Array<Heap::Stats> heapStats = Heap::GetAllHeapStats();
        long heapAllocCount = 0;
        long heapAllocSize = 0;
        IndexT i;
        for (i = 0; i < heapStats.Size(); i++)
        {
            heapAllocCount += heapStats[i].allocCount;
            heapAllocSize  += heapStats[i].allocSize;
        };
        htmlWriter->Element(HtmlElement::Heading3, "Nebula Overall Stats");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Global Heaps Alloc Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(Memory::TotalAllocCount));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Global Heaps Alloc Size: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(Memory::TotalAllocSize) + " bytes");
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Local Heaps Alloc Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(heapAllocCount));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Local Heaps Alloc Size: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(heapAllocSize) + " bytes");
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Overall Alloc Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(heapAllocCount + Memory::TotalAllocCount));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Nebula Overall Alloc Size: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromLong(heapAllocSize + Memory::TotalAllocSize) + " bytes");
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);

        // lowlevel system stats
        Memory::TotalMemoryStatus globalStats = Memory::GetTotalMemoryStatus();
        const long mega = 1024 * 1024;
        htmlWriter->Element(HtmlElement::Heading3, "System Memory Stats");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Total Physical Memory:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(globalStats.totalPhysical / mega) + String(" MB"));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Available Physical Memory:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(globalStats.availPhysical / mega) + String(" MB"));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Total Virtual Memory:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(globalStats.totalVirtual / mega) + String(" MB"));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Available Virtual Memory:");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(globalStats.availVirtual / mega) + String(" MB"));
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);

        // display global heaps info
        htmlWriter->Element(HtmlElement::Heading3, "Global Heap Stats");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Heap Name");
                htmlWriter->Element(HtmlElement::TableHeader, "Alloc Count");
                htmlWriter->Element(HtmlElement::TableHeader, "Alloc Size");
            htmlWriter->End(HtmlElement::TableRow);

            for (i = 0; i < Memory::NumHeapTypes; i++)
            {
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, Memory::GetHeapTypeName((Memory::HeapType)i));
                    htmlWriter->Element(HtmlElement::TableData, String::FromLong(Memory::HeapTypeAllocCount[i]));
                    htmlWriter->Element(HtmlElement::TableData, String::FromLong(Memory::HeapTypeAllocSize[i]));
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);

        // display local heaps info
        htmlWriter->Element(HtmlElement::Heading3, "Local Heap Stats");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Heap Name");
                htmlWriter->Element(HtmlElement::TableHeader, "Alloc Count");
                htmlWriter->Element(HtmlElement::TableHeader, "Alloc Size");
            htmlWriter->End(HtmlElement::TableRow);

            for (i = 0; i < heapStats.Size(); i++)
            {
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, heapStats[i].name);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(heapStats[i].allocCount));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(heapStats[i].allocSize));
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);

        // dump RefCounted pool allocator stats
        #if NEBULA_OBJECTS_USE_MEMORYPOOL
        
        htmlWriter->Element(HtmlElement::Heading3, "Object PoolArrayAllocator Stats");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, " Block Size ");
                htmlWriter->Element(HtmlElement::TableHeader, " Max Blocks ");
                htmlWriter->Element(HtmlElement::TableHeader, " Num Blocks ");
                htmlWriter->Element(HtmlElement::TableHeader, " Percent Full ");
            htmlWriter->End(HtmlElement::TableRow);

            for (i = 0; i < PoolArrayAllocator::NumPools; i++)
            {
                const MemoryPool& pool = ObjectPoolAllocator->GetMemoryPool(i);
                long blockSize = pool.GetBlockSize();
                long numBlocks = pool.GetNumBlocks();
                long allocCount = pool.GetAllocCount();
                uint percent = (allocCount * 100) / numBlocks;
                if (percent < 50)
                {
                    // orange if too few allocations
                    htmlWriter->AddAttr("bgcolor", "orange");
                }
                else if (percent < 75)
                {
                    // green if just right
                    htmlWriter->AddAttr("bgcolor", "green");
                }
                else
                {
                    // red if almost full
                    htmlWriter->AddAttr("bgcolor", "red");
                }
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(blockSize));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(numBlocks));
                    htmlWriter->Element(HtmlElement::TableData, String::FromLong(allocCount));
                    String percentStr;
                    percentStr.Format("%d %%", percent);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(percent));
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);


        #endif // NEBULA_OBJECTS_USE_MEMORYPOOL
        #endif // NEBULA_MEMORY_STATS
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

} // namespace Debug
