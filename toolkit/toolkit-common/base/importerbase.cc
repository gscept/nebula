//------------------------------------------------------------------------------
//  importerbase.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "base/importerbase.h"
#include "net/socket/ipaddress.h"
#include "io/ioserver.h"

#include "io/jsonreader.h"
#include "io/jsonwriter.h"

#include "db/dataset.h"
#include "db/sqlite3/sqlite3factory.h"

#include "io/assignregistry.h"
#include "core/ptr.h"

using namespace ToolkitUtil;
using namespace Net;
#if __WIN32__
//using namespace Win360;
#endif
using namespace IO;
using namespace Util;

namespace Base
{
__ImplementClass(Base::ImporterBase, 'EXBA', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ImporterBase::ImporterBase() :
	platform(Platform::Win32),
	progressCallback(0),
	minMaxCallback(0),
	hasErrors(false),
	force(false),
	isOpen(false),
	remote(true)
{
	this->socket = Socket::Create();
	this->socket->SetAddress(IpAddress("localhost", 15302));
	bool opened = this->socket->Open(Socket::UDP);
	n_assert(opened);
	Socket::Result result = this->socket->Connect();
}

//------------------------------------------------------------------------------
/**
*/
ImporterBase::~ImporterBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::Open()
{
	n_assert(!this->isOpen);
	this->isOpen = true;

    // Read the resource table and populate URN -> URI lookup map
    URI resTableUri("export:resource_mappings.sqlite");

    // Determine access mode
    IoServer* ioServer = IoServer::Instance();
    Db::Database::AccessMode accessMode = Db::Database::ReadWriteCreate;

    // Create database factory and database
    if (!Db::Sqlite3Factory::HasInstance())
    {
        this->dbFactory = Db::Sqlite3Factory::Create();
    }
    this->database = Db::DbFactory::Instance()->CreateDatabase();
    this->database->SetURI(resTableUri);
    this->database->SetAccessMode(accessMode);
    this->database->SetIgnoreUnknownColumns(false);
    this->database->SetInMemoryDatabase(false);

    bool opened = this->database->Open();
    n_assert_msg(opened, "Resource mapping database failed to open. Ensure no other process is using the file");

    // If database is new, set it up
    if (!this->database->HasTable("Mappings"))
    {
        // ==================== Create Export mappings Table ====================
        {
            Ptr<Db::Table> exportTable = Db::DbFactory::Instance()->CreateTable();
            exportTable->SetName("Mappings");
            exportTable->AddColumn(Db::Column(Attr::URNHash, Db::Column::Primary));
            exportTable->AddColumn(Db::Column(Attr::WorkHash, Db::Column::Indexed));
            exportTable->AddColumn(Db::Column(Attr::Export, Db::Column::Default));
            exportTable->AddColumn(Db::Column(Attr::Work, Db::Column::Default));

            this->database->AddTable(exportTable);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::Close()
{
	n_assert(this->isOpen);
	this->isOpen = false;
	this->socket->Close();
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::ImportFile(const IO::URI& file)
{
	// implement specific behaviour in subclass!

	// if we export a file, we should also always export an intermediate file

}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::ExportDir(const Util::String& category)
{
	// empty, implement in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::ExportAll()
{
	// empty, implement in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::Progress(float progress, const Util::String& status)
{
	if (this->remote)
	{
		/// we need tag, progress, size of string, and string itself saved
		int size = sizeof(int) + sizeof(float) + sizeof(int) + status.Length() + 1;
		char* package = new char[size];
		char* chunk = package;

		*(int*)chunk = 'QPRO';
		chunk += 4;
		*(float*)chunk = progress;
		chunk += 4;
		*(int*)chunk = status.Length();
		chunk += 4;
		bool copyStatus = status.CopyToBuffer(chunk, status.Length() + 1);
		n_assert(copyStatus);
		int sentBytes = 0;
		Socket::Result result = this->socket->Send(package, size, sentBytes);
		delete[] package;
	}
	else if (this->progressCallback)
	{
		this->progressCallback(progress, status);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::SetProgressMinMax(int min, int max)
{
	if (this->remote)
	{
		int size = sizeof(int) + sizeof(int) + sizeof(int);
		int* package = new int[size];
		int* chunk = package;

		*chunk = 'QINT';
		chunk++;
		*chunk = min;
		chunk++;
		*chunk = max;

		int sentBytes = 0;
		Socket::Result result = this->socket->Send(package, size, sentBytes);
		delete[] package;
	}
	else if (this->minMaxCallback)
	{
		this->minMaxCallback(min, max);
	}
}

//------------------------------------------------------------------------------
/**
*/
int
ImporterBase::CountExports(const Util::String& dir, const Util::String& ext)
{
	int count = 0;
	switch (this->exportFlag)
	{
		case All:
		{
			Array<String> dirs = IoServer::Instance()->ListDirectories(dir, "*");
			for (int catIndex = 0; catIndex < dirs.Size(); catIndex++)
			{
				String category = dir + "/" + dirs[catIndex];
				Array<String> files = IoServer::Instance()->ListFiles(category, "*." + ext);
				count += files.Size();
			}
			break;
		}
		case Dir:
		{
			Array<String> files = IoServer::Instance()->ListFiles(dir, "*." + ext);
			count += files.Size();
			break;
		}
        case File:
        n_error("used file as directory");
        break;
	}
	return count;
}

//------------------------------------------------------------------------------
/**
*/
bool
ImporterBase::NeedsConversion(const Util::String& src, const Util::String& dst)
{
	if (this->force)
	{
		return true;
	}

	IoServer* ioServer = IoServer::Instance();
	if (!ioServer->FileExists(src))
	{
		return false;
	}
	if (ioServer->FileExists(dst))
	{
		FileTime srcFileTime = ioServer->GetFileWriteTime(src);
		FileTime dstFileTime = ioServer->GetFileWriteTime(dst);
		if (dstFileTime > srcFileTime)
		{
			return false;
		}
	}

	// fallthrough
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::ValidateIntermediateFile(String const& filePath)
{
	IoServer* ioServer = IoServer::Instance();

	// first check if the source file has been removed, if so, we read the intermediate file and remove all exported data.
	constexpr char fileExt[] = ".export";
	constexpr SizeT numCharsToStrip = (sizeof(fileExt) / sizeof(char)) - 1;

	String sourceFilePath = filePath.ExtractRange(0, filePath.Length() - numCharsToStrip);
	sourceFilePath.SubstituteString("intermediate:", "src:");

	if (!ioServer->FileExists(sourceFilePath))
	{
		IO::URI const file = filePath;
		Ptr<JsonReader> reader = JsonReader::Create();
		reader->SetStream(ioServer->CreateStream(file));
		if (reader->Open())
		{
			reader->SetToFirstChild("exported_files");
			if (reader->IsArray())
			{
				Util::Array<Util::String> exportedFiles;
				reader->Get<>(exportedFiles);
				for (Util::String const& exportedFile : exportedFiles)
				{
					URI uri = exportedFile;
					// Hopefully avoids stupid mistakes
					if (uri.IsEmpty() || !uri.IsValid() || exportedFile == "/" || !ioServer->FileExists(uri))
						continue;

					ioServer->DeleteFile(uri);
				}
			}
			reader->SetToParent();
		}
		reader->Close();
		// delete the intermediate file
		ioServer->DeleteFile(file);
	}
}

//------------------------------------------------------------------------------
/**
	Check if any files have been deleted or moved in our work directory as the exported files then needs to be removed.
*/
void
ImporterBase::RecurseValidateIntermediates(String const& dir)
{
	
	IoServer* ioServer = IoServer::Instance();
	Util::Array<String> files = ioServer->ListFiles(dir, "*.export");

	for (IndexT i = 0; i < files.Size(); i++)
	{
		this->ValidateIntermediateFile(dir + files[i]);
	}

	Util::Array<String> directories = ioServer->ListDirectories(dir, "*");
	for (IndexT i = 0; i < directories.Size(); i++)
	{
		this->RecurseValidateIntermediates(dir + directories[i] + "/");
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ImporterBase::WriteIntermediateFile(const IO::URI& sourceFile, Util::Array<IO::URI> const& output)
{
	IO::URI const assetPath = "src:";
	Ptr<JsonWriter> writer = JsonWriter::Create();
	Util::String intermediateFile = sourceFile.LocalPath();
	intermediateFile.SubstituteString(assetPath.LocalPath(), "intermediate:");
	intermediateFile.Append(".export");
	Ptr<IoServer> ioServer = IoServer::Instance();
	IO::URI intermediateDir = intermediateFile.ExtractDirName();
	if (!ioServer->DirectoryExists(intermediateDir))
	{
		ioServer->CreateDirectory(intermediateDir);
	}
	writer->SetStream(ioServer->CreateStream(intermediateFile));
	if (writer->Open())
	{
		writer->BeginArray("exported_files");
		for (IndexT i = 0; i < output.Size(); i++)
		{
			writer->Add(output[i].GetTail());
		}
		writer->End();
		writer->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ImporterBase::UpdateResourceMapping(Util::String urn, Util::String work, Util::String exp)
{
    Ptr<Db::Table> exportMappings = this->database->GetTableByName("Mappings");
    Ptr<Db::Dataset> exportDataset = exportMappings->CreateDataset();   
    Ptr<Db::FilterSet> filter = exportDataset->Filter();

    exportDataset->AddColumn(Attr::URNHash);
    exportDataset->AddColumn(Attr::WorkHash);
    exportDataset->AddColumn(Attr::Export);
    exportDataset->AddColumn(Attr::Work);

    int urnHash = urn.HashCode();
    filter->AddEqualCheck(Attr::Attribute(Attr::URNHash, urnHash));
    exportDataset->PerformQuery();
    Ptr<Db::ValueTable> exportValues = exportDataset->Values();
    IndexT rowIdx = 0;
    if (exportValues->GetNumRows() == 0)
    {
        exportValues->AddRow();
        rowIdx = exportValues->GetNumRows() - 1;
    }

    exportValues->AddRow();
    exportValues->SetInt(Attr::URNHash, rowIdx, urnHash);
    exportValues->SetInt(Attr::WorkHash, rowIdx, work.HashCode());
    exportValues->SetString(Attr::Export, rowIdx, exp);
    exportValues->SetString(Attr::Work, rowIdx, work);

    exportDataset->CommitChanges();
}


} // namespace ToolkitUtil