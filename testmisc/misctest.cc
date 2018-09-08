//------------------------------------------------------------------------------
// visibilitytest.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/refcounted.h"
#include "timing/timer.h"
#include "io/console.h"
#include "misctest.h"
#include "app/application.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "io/ioserver.h"

#include "memory/ringallocator.h"
#include "threading/safequeue.h"

#include <thread>

const char* lorem_ipsum = "\
Lorem ipsum dolor sit amet, aut sit fugiat fusce tempor, ac tempor lorem vel quis at, consectetuer vestibulum libero volutpat, sit est praesent amet lacus.Adipiscing mollis id neque adipiscing sem, sed maecenas pellentesque.Fringilla eget et lectus dictumst, vehicula at ullamcorper.Vehicula odio ac luctus pellentesque tempus, morbi id nonummy iaculis vestibulum turpis, cubilia orci vulputate illum etiam.Nulla integer mollis facilisis ullamcorper, ac laoreet interdum donec justo facilisis in, feugiat enim nibh.Wisi quam nulla egestas, quis justo arcu, non interdum.\
\
Imperdiet nonummy dolor, platea duis rutrum elit nam arcu, corrupti penatibus vitae vitae ac non nec.Sit aliquam orci nec non wisi, quis nec elit dignissim magna viverra, velit mi eu wisi, est non risus a fusce etiam ultrices.Nam mauris donec odio non, metus justo, vitae faucibus sit justo, malesuada pellentesque qui metus tristique duis.Donec ridiculus accumsan sodales viverra a turpis, eu rutrum vitae, augue egestas, morbi orci, imperdiet luctus ipsum sapien.Proin vitae orci leo, dui malesuada montes id, pede vitae vel, nulla tempus, vestibulum etiam odio augue aliquam bibendum.\
\
Tellus augue congue.Dolor justo risus, eu purus, nunc tincidunt suspendisse sit, ligula purus est.Libero similique vivamus, mollis ullamcorper nec ut ut lectus, dolor risus pharetra ultricies amet adipiscing.Viverra at neque quam metus sem, lorem integer tempor blandit nam et, metus et vestibulum.Urna justo nisl, wisi at turpis sodales parturient, suscipit fusce elit curabitur metus eleifend enim, luctus luctus eget, wisi eleifend ipsum tincidunt commodo.Neque enim dolor, quis tortor, wisi sollicitudin lorem eros consequat id, bibendum montes.Etiam a amet, velit amet ante donec, volutpat augue tristique quis ut.Magna suscipit, ac ridiculus tellus, dui sit vitae quisque nonummy orci sit.Iaculis ut velit quam sagittis vestibulum iaculis, aenean ac nunc sit, quis libero lacinia tincidunt sed lacinia felis, eu montes lacus commodo.Rhoncus nullam id, consequat est, quam magna quis nibh non libero cras, sed posuere nec, ante eu blandit purus aliquet.\
\
Nisl pretium at semper tincidunt.Elit egestas massa mauris sed potenti varius, eleifend elit in posuere sem, scelerisque viverra lacinia lacinia.Felis laoreet pede cras eu, praesent deserunt, at risus in enim ac justo, placerat mauris accumsan sapien tempor.Arcu vivamus ut tristique leo, eget non vel mauris curabitur felis hymenaeos, placerat sed, in pulvinar eius placerat, dui posuere justo euismod nisl sodales.Dignissim felis orci lorem.\
";

using namespace Timing;
namespace Test
{


__ImplementClass(MiscTest, 'RETE', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
void
MiscTest::Run()
{
	App::Application app;

	Ptr<IO::IoServer> ioServer = IO::IoServer::Create();

	app.SetAppTitle("Misc test!");
	app.SetCompanyName("gscept");
	app.Open();

	// 1 MB
	Memory::RingAllocator<8> alloc(1024*1024);

	struct Command
	{
		enum Type
		{
			DoWork,
			Sync
		};

		union
		{
			struct
			{
				Threading::Event* ev;
			} ev;

			struct
			{
				uint offset;
				byte* buf;
			} work;
		};

		Type type;
	};
	Threading::SafeQueue<Command> events;
	byte* buffer = n_new_array(byte, 1024 * 1024);

	static const uint NumWork = 5;
	static const uint WorkSize = 512;

	std::thread th([&alloc, &events, buffer]()
	{
		// create buffer for data which we will copy to
		while (true)
		{
			Util::Array<Command> commands;
			events.DequeueAll(commands);

			for (IndexT i = 0; i < commands.Size(); i++)
			{
				const Command& cmd = commands[i];
				switch (cmd.type)
				{
				case Command::DoWork:
					memcpy(buffer + cmd.work.offset, cmd.work.buf, WorkSize);
					break;
				case Command::Sync:
					cmd.ev.ev->Signal();
					break;	
				}
			}

			buffer[WorkSize * NumWork] = '\0';
			printf("%s", buffer);
			events.Wait();
		}
		
	});


	while (true)
	{
		// start allocation and check for the next upcoming range to be freed
		alloc.Start();

		// go through and generate jobs
		for (IndexT i = 0; i < NumWork; i++)
		{
			// allocate buffer region, this should not be accessible until the
			// thread is done working with this memory
			Memory::RingAlloc buf;
			if (alloc.Allocate(WorkSize, buf))
			{
				// copy to the offset data pointer, offset by our text sample
				memcpy(buf.data, lorem_ipsum + i * WorkSize, WorkSize);

				// send a command to the thread to copy the data to the thread buffer
				Command cmd;
				cmd.type = Command::DoWork;
				cmd.work.buf = buf.data;
				cmd.work.offset = buf.offset;
			}
		}

		auto ev = alloc.End();

		if (ev)
		{
			// push sync command
			Command cmd;
			cmd.type = Command::Sync;
			cmd.ev.ev = ev;
			events.Enqueue(cmd);
		}
	}
	

	app.Close();
}

} // namespace Test