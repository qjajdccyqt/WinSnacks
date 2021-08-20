#pragma once
#include <thread>
#include <iostream>

#include <public_utils/string/charset_utils.h>

#include "krabs/client.hpp"
#include "krabs/parser.hpp"
#include "krabs/kernel_providers.hpp"

static void setup_disk_provider(krabs::provider<>& provider)
{
    // user_trace providers typically have any and all flags, whose meanings are
    // unique to the specific providers that are being invoked. To understand these
    // flags, you'll need to look to the ETW event producer.
    //provider.any(0x8000000000000000);

    // providers should be wired up with functions (or functors) that are called when
    // events from that provider are fired.
    provider.add_on_event_callback([](const EVENT_RECORD& record, const krabs::trace_context& trace_context)
    {
        try
        {
            // Once an event is received, if we want krabs to help us analyze it, we need
            // to snap in a schema to ask it for information.
            krabs::schema schema(record, trace_context.schema_locator);

            // We then have the ability to ask a few questions of the event.
            auto pid = schema.process_id();
            auto eventID = schema.event_id();
            auto eventName = schema.event_name();
            auto provider = schema.provider_name();
            auto operation = schema.event_opcode();
            if (10 == eventID || 11 == eventID)
            {
                // The event we're interested in has a field that contains a bunch of
                // info about what it's doing. We can snap in a parser to help us get
                // the property information out.
                krabs::parser parser(schema);
                // We have to explicitly name the type that we're parsing in a template
                // argument.
                // We could alternatively use try_parse if we didn't want an exception to
                // be thrown in the case of failure.
                auto diskNumber = parser.parse<uint32_t>(L"DiskNumber");
                auto transferSize = parser.parse<uint32_t>(L"TransferSize");
                if (10 == eventID)//read
                {
                    std::cout << transferSize << " bytes read from disk " << diskNumber << " PID:" << pid << std::endl;
                }
                else if (11 == eventID)//write
                {
                    std::cout << transferSize << " bytes write from disk " << diskNumber << " PID:" << pid << std::endl;
                }
            }
            else//ohter
            {
                //std::cout << "eventID:" << eventID << " eventName:" << eventName << " provider:" << provider << " operation:" << operation ;
            }
        }
        catch (const std::runtime_error& err)
        {
            std::cerr << err.what() << std::endl;
        }
    });
}

static void setup_image_load_provider(krabs::kernel::image_load_provider& provider)
{
    // Kernel providers accept all the typical callback mechanisms.
    provider.add_on_event_callback([](const EVENT_RECORD& record, const krabs::trace_context& trace_context)
    {
        krabs::schema schema(record, trace_context.schema_locator);

        // Opcodes can be found on the kernel provider's documentation:
        // https://msdn.microsoft.com/en-us/library/windows/desktop/aa364068(v=vs.85).aspx
        if (schema.event_opcode() == 10)
        {
            krabs::parser parser(schema);
            std::wstring filename = parser.parse<std::wstring>(L"FileName");
            std::cout << "Loaded image from file " << PublicUtils::CharsetUtils::UnicodeToUTF8(filename) << std::endl;
        }
    });
}

// 需要使用管理员权限运行
void main()
{
    try
    {
        // user_trace instances should be used for any non-kernel traces that are defined
        // by components or programs in Windows. You can have multiple ETW traces in a given
        // program but each trace object will consume one thread.
        krabs::user_trace user(L"disk-io");
        krabs::kernel_trace kernel(L"image-load");

        // A trace can have any number of providers, which are identified by GUID or
        // a specific trace name.
        //
        // The GUIDs are defined by the components that emit events, and their GUIDs can
        // usually be found with various ETW tools (like wevutil or Microsoft Message Analyzer).
        krabs::provider<> disk_provider(krabs::guid(L"{C7BDE69A-E1E0-4177-B6EF-283AD1525271}"));
        krabs::kernel::image_load_provider image_load_provider;

        //setup_ps_provider(ps_provider);
        setup_disk_provider(disk_provider);
        setup_image_load_provider(image_load_provider);

        // The user_trace needs to know about the provider that we've set up.
        // You can assign multiple providers to a single trace.
        user.enable(disk_provider);
        kernel.enable(image_load_provider);

        // Begin listening for events. This call blocks, so if you want to do other things
        // while this runs, you'll need to call this on another thread.
        //
        // Additionally, if multiple threads are enabling providers with a single trace object,
        // you'll need to synchronize the call to start. Because 'start' is a blocking call,
        // it will prevent any other thread from enabling additional providers.
        std::thread user_thread([&user]()
        {
            try
            {
                user.start();
            }
            catch (const std::runtime_error& err)
            {
                std::cerr << err.what() << std::endl;
                user.stop();
            }
        });
        std::thread kernel_thread([&kernel]()
        {
            try
            {
                kernel.start();
            }
            catch (const std::runtime_error& err)
            {
                std::cerr << err.what() << std::endl;
                kernel.stop();
            }
        });

        while (true)
        {
            Sleep(1000);
        }
        std::cout << "stopping traces..." << std::endl;
        user.stop();
        kernel.stop();
        user_thread.join();
        kernel_thread.join();
    }
    catch (const std::runtime_error& err)
    {
        std::cerr << err.what() << std::endl;
    }
}
