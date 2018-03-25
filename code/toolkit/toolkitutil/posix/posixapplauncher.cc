//------------------------------------------------------------------------------
//  posixapplauncher.cc
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "posixapplauncher.h"
#include <sys/wait.h>

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
PosixAppLauncher::PosixAppLauncher() :
    pid(-1),
    inPipe(-1),
    outPipe(-1),
    errPipe(-1)
{     
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixAppLauncher::LaunchWait() const
{
    if(this->Launch())
    {
        int status;
        if(waitpid(this->pid,&status,0) > 0)
        {
            this->CleanUp();
            return true;
        }
        else
        {
            this->CleanUp();
            n_error("Failed on waitpid\n");            
            return false;
        }
    }
    return false;        
}

//------------------------------------------------------------------------------
/**
    Launch the application process and returns immediately. The state of the launched
    process can be checked by calling IsRunning().
*/
bool
PosixAppLauncher::Launch() 
{        
    n_assert(this->exePath.IsValid());
             
    int in[2];
    int out[2];
    int err[2];
    int pid;
    int rc;

    rc = pipe(in);
    
    if(rc<0)
    {
        n_error("Failed to create input pipe\n");
        return false;
    }                            

    rc = pipe(out);
    if(rc<0)
    {
        n_error("Failed to create output pipe\n");
        close(in[0]);
        close(in[1]);
        return false;
    }                          

    rc = pipe(err);
    if(rc<0)    
    {
        n_error("Failed to create stderr pipe\n");
        close(in[0]);
        close(in[1]);
        close(out[0]);
        close(out[1]);
        return false;
    }

    pid = fork();
    if(pid > 0)
    { // parent
            close(in[0]);
            close(out[1]);
            close(err[1]);
            inPipe = in[1];
            outPipe = out[0];
            errPipe = err[0];
            
            // we are currently not using stdin so we close it again
            close(inPipe);
            if(!this->stdoutCaptureStream.isvalid())
            {
                close(outPipe);
            }    
            else
            {
                this->stdoutCaptureStream->SetAccessMode(Stream::WriteAccess);
                this->stdoutCaptureStream->Open();
            }
            
            if(!this->stderrCaptureStream.isvalid())
            {
                close(errPipe);
            }        
            else
            {
                this->stderrCaptureStream->SetAccessMode(Stream::WriteAccess);
                this->stderrCaptureStream->Open();
            }
            this->isRunning = true;
            
            return true;
     
    }
    else if(pid == 0)
    { // child
            close(in[1]);
            close(out[0]);
            close(err[0]);
            close(0);
            dup(in[0]);
            close(1);
            dup(out[1]);
            close(2);
            dup(err[1]);
            
            // convert arg string to array of char*
            Array<String> strargs = this->args.Tokenize(" ",'\"');
            char ** argv = new char*[strargs.Size() + 2];
            argv[0] = this->exePath.LocalPath().AsCharPtr();
            int i;
            for(i = 0 ; i<strargs.Size() ; i++)
            {
                argv[i+1] = strargs[i].AsCharPtr();
            }
            argv[i+1] = NULL;
                

            execvp(this->exePath.LocalPath().AsCharPtr(), (char**)argv);
            exit(1);
    }
    else
    {        
        close(err[0]);
        close(err[1]);
        close(out[0]);
        close(out[1]);
        close(in[0]);
        close(in[1]);
        n_error("Failed to fork\n");
        return false;
    }         
}

//------------------------------------------------------------------------------
/**
    Gets the state of the application.
*/
bool
PosixAppLauncher::IsRunning()
{
    if(this->isRunning)
    {        
        this->UpdateStdoutStream();
        int status;
        int res = waitpid(this->pid, &status, WNOHANG);
        if(res > 0)
        {
            // cleanup
            if(this->stdoutCaptureStream.isvalid())
            {
                this->stdoutCaptureStream->Close();                
            }
            if(this->stderrCaptureStream.isvalid())
            {
                this->stderrCaptureStream->Close();                
            }            
            this->CleanUp();
            this->isRunning = false;
        }        
    }    
    return this->isRunning;
}
//------------------------------------------------------------------------------
/**
    Reads all arrived data from stdout since the last call of this method and puts it to
    the stream.
*/
void
PosixAppLauncher::UpdateStdoutStream()
{
    static char buffer[256];
    if(this->stdoutCaptureStream.isvalid())
    {
        int bread = read(outPipe,buffer,256) > 0;
        while(bread > 0)
        {
            this->stdoutCaptureStream->Write(buffer,bread);
            bread = read(outPipe,buffer,256) > 0;
        }
    }
    if(this->stderrCaptureStream.isvalid())
    {
        int bread = read(errPipe,buffer,256) > 0;
        while(bread > 0)
        {
            this->stderrCaptureStream->Write(buffer,bread);
            bread = read(errPipe,buffer,256) > 0;
        }                
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixAppLauncher::CheckIfExists()
{
    //FIXME: not implemented
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
PosixAppLauncher::CheckIfExists(const IO::URI & program)
{
    //FIXME: not implemented
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
PosixAppLauncher::CleanUp() const
{
    close(outPipe);
    close(errPipe);
}

} // namespace ToolkitUtil
