//------------------------------------------------------------------------------
//  processtest.cc
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "processtest.h"
#include "system/process.h"
#include "io/memorystream.h"

namespace Test
{
__ImplementClass(Test::ProcessTest, 'PRCT', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
ProcessTest::Run()
{
#if __WIN32__
    const IO::URI shell("C:/Windows/System32/cmd.exe");
    const Util::String outputCommand = "/C \"echo stdout & echo stderr 1>&2 & exit /B 7\"";
    const Util::String delayedCommand = "/C \"ping 127.0.0.1 -n 2 > NUL & exit /B 3\"";
#else
    const IO::URI shell("/bin/sh");
    const Util::String outputCommand = "-c \"printf stdout; printf stderr >&2; exit 7\"";
    const Util::String delayedCommand = "-c \"sleep 1; exit 3\"";
#endif

    System::ProcessStartInfo invalidInfo = {};
    invalidInfo.exePath = IO::URI("non-existing-process");
    invalidInfo.consoleWindow = false;
    VERIFY(System::StartProcess(invalidInfo) == System::InvalidProcessId);

    Ptr<IO::MemoryStream> stdoutStream = IO::MemoryStream::Create();
    Ptr<IO::MemoryStream> stderrStream = IO::MemoryStream::Create();
    System::ProcessStartInfo outputInfo = {};
    outputInfo.exePath = shell;
    outputInfo.args = outputCommand;
    outputInfo.consoleWindow = false;
    outputInfo.outputStream = stdoutStream.upcast<IO::Stream>();
    outputInfo.errorStream = stderrStream.upcast<IO::Stream>();

    System::ProcessId outputProcess = System::StartProcess(outputInfo);
    VERIFY(outputProcess != System::InvalidProcessId);
    VERIFY(System::WaitForProcess(outputProcess) == 7);
    VERIFY(stdoutStream->GetSize() == 6);
    VERIFY(stderrStream->GetSize() == 6);
    VERIFY(memcmp(stdoutStream->GetRawPointer(), "stdout", 6) == 0);
    VERIFY(memcmp(stderrStream->GetRawPointer(), "stderr", 6) == 0);

#if !__WIN32__
    Ptr<IO::MemoryStream> workingDirectoryStream = IO::MemoryStream::Create();
    System::ProcessStartInfo workingDirectoryInfo = {};
    workingDirectoryInfo.exePath = shell;
    // Run pwd and double-check that the workingdir is actually correct after 
    // spawning the application.
    workingDirectoryInfo.args = "-c \"pwd\"";
    workingDirectoryInfo.workingDir = IO::URI("/tmp");
    workingDirectoryInfo.consoleWindow = false;
    workingDirectoryInfo.outputStream = workingDirectoryStream.upcast<IO::Stream>();
    System::ProcessId workingDirectoryProcess = System::StartProcess(workingDirectoryInfo);
    VERIFY(workingDirectoryProcess != System::InvalidProcessId);
    VERIFY(System::WaitForProcess(workingDirectoryProcess) == 0);
    VERIFY(workingDirectoryStream->GetSize() == 5);
    VERIFY(memcmp(workingDirectoryStream->GetRawPointer(), "/tmp\n", 5) == 0);
#endif

    System::ProcessStartInfo delayedInfo = {};
    delayedInfo.exePath = shell;
    delayedInfo.args = delayedCommand;
    delayedInfo.consoleWindow = false;
    System::ProcessId delayedProcess = System::StartProcess(delayedInfo);
    VERIFY(delayedProcess != System::InvalidProcessId);
    VERIFY(System::IsProcessRunning(delayedProcess));
    VERIFY(System::WaitForProcess(delayedProcess) == 3);
}
} // namespace Test
