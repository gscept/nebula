//------------------------------------------------------------------------------
//  texgenapp.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "texgenapp.h"
#include "toolkit-common/platform.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/console.h"
#include "io/uri.h"
#include "timing/time.h"
#include <IL/il.h>
#include <IL/ilu.h>

namespace Toolkit
{
using namespace ToolkitUtil;
using namespace Util;
using namespace IO;

TexGenApp::TexGenApp() :
    ToolkitApp(),
    outputWidth(0),
    outputHeight(0),
    outputChannels(0)
{
    this->BGRA[0] = -1.0f;
    this->BGRA[1] = -1.0f;
    this->BGRA[2] = -1.0f;
    this->BGRA[3] = -1.0f;
}

//------------------------------------------------------------------------------
/**
*/
void
TexGenApp::Run()
{
    IoServer* ioServer = IoServer::Instance();

    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

    Util::String inputPath = this->inputFile;

    ILuint outputImage;
    ilGenImages(1, &outputImage);
    
    ILint imgWidth = this->outputWidth;
    ILint imgHeight = this->outputHeight;
    
    ILint numChannels = this->outputChannels;
    ILenum format = 0;
    switch (numChannels)
    {
    case 1:
        format = IL_RED;
        break;
    case 2:
        format = IL_RG;
        break;
    case 3:
        format = IL_RGB;
        break;
    case 4:
        format = IL_RGBA;
        break;
    }
    ILenum type = 0;

    bool initialized = false;
    auto InitializeImage = [&initialized, &imgWidth, &imgHeight, &numChannels, &format, &type](ILuint outputImage, ILuint inputImage, float inputWidth, float inputHeight, int inputChannels, ILenum inputFormat)
    {
        imgWidth = imgWidth > 0 ? imgWidth : inputWidth;
        imgHeight = imgHeight > 0 ? imgHeight : inputHeight;
        numChannels = numChannels > 0 ? numChannels : inputChannels;
        format = format > 0 ? format : inputFormat;

        type = ilGetInteger(IL_IMAGE_TYPE);

        ilBindImage(outputImage);
        ilTexImage(imgWidth, imgHeight, 1, numChannels, format, type, NULL);
        initialized = true;
        ilBindImage(NULL);
    };

    // first, load possible input image
    if (!this->inputFile.IsEmpty())
    {
        ILuint inputImage;
        ilGenImages(1, &inputImage);
        ilBindImage(inputImage);
        
        ILboolean res = ilLoadImage((ILstring)this->inputFile.AsCharPtr());
        if (IL_TRUE != res)
        {
            this->logger.Print("Could not load input file!\n");
            this->SetReturnCode(1);
            ilDeleteImages(1, &inputImage);
            ilDeleteImages(1, &outputImage);
            return;
        }

        ILint inputWidth = ilGetInteger(IL_IMAGE_WIDTH);
        ILint inputHeight = ilGetInteger(IL_IMAGE_HEIGHT);
        ILint inputChannels = ilGetInteger(IL_IMAGE_CHANNELS);
        ILenum format = ilGetInteger(IL_IMAGE_FORMAT);

        InitializeImage(outputImage, inputImage, inputWidth, inputHeight, inputChannels, format);
        ilBindImage(outputImage);
        iluImageParameter(ILU_FILTER, ILU_SCALE_BOX);
        ilCopyImage(inputImage);
        iluScale(imgWidth, imgHeight, 1);
        ilDeleteImages(1, &inputImage);
        ilBindImage(NULL);
    }

    // for each channel-specific image or color, override channel
    for (int channel = 0; channel < numChannels; channel++)
    {
        if (!this->outputBGRA[channel].IsEmpty())
        {
            Util::Array<Util::String> tokens = this->outputBGRA[channel].Tokenize(":");
            if (tokens.Size() < 2)
            {
                this->logger.Print("invalid input specified for channel specific images!\n");
                this->SetReturnCode(1);
                return;
            }

            Util::String& filePath = tokens[0];
            Util::String& channelCharacter = tokens[1];
            int inputChannelIndex = 0;
            if (channelCharacter == "b")
                inputChannelIndex = 0;
            else if (channelCharacter == "g")
                inputChannelIndex = 1;
            else if (channelCharacter == "r")
                inputChannelIndex = 2;
            else if (channelCharacter == "a")
                inputChannelIndex = 3;
            else
            {
                this->logger.Print("invalid channel specified for channel-specific images!\n");
                this->SetReturnCode(1);
                return;
            }

            ILuint channelImage;
            ilGenImages(1, &channelImage);
            ilBindImage(channelImage);

            ILboolean res = ilLoadImage((ILstring)filePath.AsCharPtr());
            if (IL_TRUE != res)
            {
                this->logger.Print("Could not load channel input file!\n");
                this->SetReturnCode(1);
                ilDeleteImages(1, &channelImage);
                ilDeleteImages(1, &outputImage);
                return;
            }

            ILint channelWidth = ilGetInteger(IL_IMAGE_WIDTH);
            ILint channelHeight = ilGetInteger(IL_IMAGE_HEIGHT);
            ILint numInputChannels = ilGetInteger(IL_IMAGE_CHANNELS);

            if (!initialized)
            {
                InitializeImage(outputImage, channelImage, imgWidth, imgHeight, numChannels, format);
            }

            ilBindImage(channelImage);
            ILubyte* in = ilGetData();
            ilBindImage(outputImage);
            ILubyte* out = ilGetData();

            float const dW = (float)channelWidth / (float)imgWidth;
            float const dH = (float)channelHeight / (float)imgHeight;

            for (int y = 0; y < imgHeight; y++)
            {
                for (int x = 0; x < imgWidth; x++)
                {
                    int const oPixel = (x + y * imgWidth) * numChannels;
                    int const iPixel = ((int)(x * dW) + (int)(y * dH) * channelWidth) * numInputChannels;
                    out[oPixel + channel] = in[iPixel + inputChannelIndex];
                }
            }
            ilBindImage(NULL);
            ilDeleteImages(1, &channelImage);
        }
    }

    for (int channel = 0; channel < numChannels; channel++)
    {
        if (this->outputBGRA[channel].IsEmpty())
        {
            if (this->BGRA[channel] != -1.0f)
            {
                // override with color
                ilBindImage(outputImage);
                ILubyte* out = ilGetData();

                for (int y = 0; y < imgHeight; y++)
                {
                    for (int x = 0; x < imgWidth; x++)
                    {
                        int pixel = (x + y * imgWidth) * numChannels;
                        uint val = (uint)(this->BGRA[channel] * 255.f);
                        out[pixel + channel] = (ILubyte)val;
                    }
                }
            }
            else if (this->inputFile.IsEmpty())
            {
                // override with zeroes
                ilBindImage(outputImage);
                ILubyte* out = ilGetData();

                for (int y = 0; y < imgHeight; y++)
                {
                    for (int x = 0; x < imgWidth; x++)
                    {
                        int pixel = (x + y * imgWidth) * numChannels;
                        out[pixel + channel] = 0;
                    }
                }
            }
        }
    }
    
    ilBindImage(outputImage);
    ilEnable(IL_FILE_OVERWRITE);
    if (ilSaveImage(this->outputFile.AsCharPtr()))
    {
        this->logger.Print("Image generated to %s\n", this->outputFile.AsCharPtr());
    }
    else
    {
        this->logger.Print("Failed to generate image to %s\n", this->outputFile.AsCharPtr());
        this->SetReturnCode(1);
        return;
    }

    ilDeleteImages(1, &outputImage);
}

//------------------------------------------------------------------------------
/**
*/
bool TexGenApp::ParseCmdLineArgs()
{
    if (ToolkitApp::ParseCmdLineArgs())
    {
        if (!this->args.HasArg("-o"))
        {
            this->logger.Print("No output file specified!\n");
            this->SetReturnCode(1);
            return false;
        }
        this->outputFile = this->args.GetString("-o");
        
        if (this->args.HasArg("-i"))
        {
            Util::Array<Util::String> inputs = this->args.GetStrings("-i");

            for (IndexT i = 0; i < inputs.Size(); i++)
            {
                Util::String input = inputs[i];
                Util::Array<Util::String> swizzle = input.Tokenize("?");
                this->inputFile = swizzle.Size() == 0 ? swizzle[0] : "";
                if (swizzle.Size() > 0)
                {
                    Util::String file = swizzle[0];

                    // channels are split into source:target
                    Util::Array<Util::String> channels = swizzle[1].Tokenize(":");
                    if (channels.Size() != 2)
                    {
                        this->logger.Print("If using channel swizzling, must specify (r|g|b|a):(r|g|b|a)");
                        return false;
                    }
                    if (channels[0].Length() > 4 || channels[1].Length() > 4)
                    {
                        this->logger.Print("Too many channels specified '%s' or '%s'", channels[0].AsCharPtr(), channels[1].AsCharPtr());
                        return false;
                    }
                    if (channels[0].Length() != channels[1].Length())
                    {
                        this->logger.Print("Swizzle arguments must be of equal size, but provided '%d';'%d'", channels[0].Length(), channels[1].Length());
                        return false;
                    }
                    
                    for (int j = 0; j < channels[0].Length(); j++)
                    {
                        char c = channels[1][j];
                        switch (c)
                        {
                        case 'r':
                        case 'R':
                            if (!this->outputBGRA[2].IsEmpty())
                            {
                                this->logger.Print("Red channel already occupied by '%s'", this->outputBGRA[2].AsCharPtr());
                                return false;
                            }
                            else
                                this->outputBGRA[2] = Util::String::Sprintf("%s:%c", file.AsCharPtr(), channels[1][j]);
                            break;
                        case 'g':
                        case 'G':
                            if (!this->outputBGRA[1].IsEmpty())
                            {
                                this->logger.Print("Green channel already occupied by '%s'", this->outputBGRA[1].AsCharPtr());
                                return false;
                            }
                            else
                                this->outputBGRA[1] = Util::String::Sprintf("%s:%c", file.AsCharPtr(), channels[1][j]);
                            break;
                        case 'b':
                        case 'B':
                            if (!this->outputBGRA[0].IsEmpty())
                            {
                                this->logger.Print("Blue channel already occupied by '%s'", this->outputBGRA[0].AsCharPtr());
                                return false;
                            }
                            else
                                this->outputBGRA[0] = Util::String::Sprintf("%s:%c", file.AsCharPtr(), channels[1][j]);
                            break;
                        case 'a':
                        case 'A':
                            if (!this->outputBGRA[3].IsEmpty())
                            {
                                this->logger.Print("Alpha channel already occupied by '%s'", this->outputBGRA[3].AsCharPtr());
                                return false;
                            }
                            else
                                this->outputBGRA[3] = Util::String::Sprintf("%s:%c", file.AsCharPtr(), channels[1][j]);
                            break;
                        }
                    }
                }
            }
        }

        constexpr const char* bgra = "bgra";
        for (int i = 0; i < 4; i++)
        {
            Util::String arg;
            arg.Format("-%c", bgra[i]);
            if (this->args.HasArg(arg))
            {
                Util::String val = this->args.GetString(arg);
                if (val.IsValidFloat() || val.IsValidInt())
                {
                    this->BGRA[i] = val.AsFloat();
                }
                else
                {
                    this->logger.Print("When specifying a single channel value, must be float or int");
                }
            }
        }

        this->outputWidth = this->args.GetInt("-w", 0);
        this->outputHeight = this->args.GetInt("-h", 0);
        this->outputChannels = this->args.GetInt("-c", 0);

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
TexGenApp::ShowHelp()
{
    n_printf("Nebula texture generator %s\n"
            "[Toolkit %s]\n"
            "(C) 2020 Individual Authors, see AUTHORS file.\n\n", this->GetAppVersion().AsCharPtr(), this->GetToolkitVersion().AsCharPtr());
    n_printf("Available options:"
             "-help                                         -- Print this help message\n"
             "-o <file>                                     -- Output file path and extension\n"
             "-c [channels]                                 -- Output file's channels.\n"
             "-w [width]                                    -- Output file's width. Will infer from input if not specified.\n"
             "-h [height]                                   -- Output file's height. Will infer from input if not specified.\n"
             "-i <file> [?(rR|gG|bB|aA):(rR|gG|bB|aA)]      -- Input file path and extension, optionally use ? to specify source and target channels\n"
             "-(r|g|b|a) [0, 1]                             -- Set a channel to a value between 0 and 1. Takes priority over '-i'\n"
             "                                Can be specified multiple times.\n"
             "                                Takes priority over '-(r/g/b/a)' and '-i'\n\n"
             "Example: texgen -o foo.bmp -i bar.bmp -o:r gnyrf.bmp:b -a 0.5\n"
             "     - This will create output file foo.bmp, using bar.bmp as base, overriding the red channel with the blue channel from gnyrf.bmp and settings the alpha channel to 0.5 across the entire image.\n\n"
    );
}


} // namespace ToolkitUtil
