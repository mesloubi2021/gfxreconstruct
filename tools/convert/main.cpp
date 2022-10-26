/*
** Copyright (c) 2018-2022 Valve Corporation
** Copyright (c) 2018-2022 LunarG, Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
*/

#include "project_version.h"
#include "tool_settings.h"
#include "format/format.h"
#include "util/platform.h"

#include "generated/generated_vulkan_ascii_consumer.h"
#if defined(WIN32)
#include "generated/generated_dx12_ascii_consumer.h"
#endif

const char kOptions[] = "-h|--help,--version,--no-debug-popup,--json-object";

const char kArguments[] = "--output"
#if defined(WIN32)
                          ",--api"
#endif
    ;

static void PrintUsage(const char* exe_name)
{
    std::string app_name     = exe_name;
    size_t      dir_location = app_name.find_last_of("/\\");
    if (dir_location >= 0)
    {
        app_name.replace(0, dir_location + 1, "");
    }
    GFXRECON_WRITE_CONSOLE("\n%s - A tool to convert GFXReconstruct capture files to JSON Lines text.\n",
                           app_name.c_str());
    GFXRECON_WRITE_CONSOLE("Usage:");
    GFXRECON_WRITE_CONSOLE("  %s [-h | --help] [--version] <file>\n", app_name.c_str());
    GFXRECON_WRITE_CONSOLE("Required arguments:");
    GFXRECON_WRITE_CONSOLE("  <file>\t\tPath to the GFXReconstruct capture file to be converted");
    GFXRECON_WRITE_CONSOLE("        \t\tto text.");
    GFXRECON_WRITE_CONSOLE("\nOptional arguments:");
    GFXRECON_WRITE_CONSOLE("  -h\t\t\tPrint usage information and exit (same as --help).");
    GFXRECON_WRITE_CONSOLE("  --version\t\tPrint version information and exit.");
    GFXRECON_WRITE_CONSOLE("  --output file\t\t'stdout' or a path to a file to write JSON output");
    GFXRECON_WRITE_CONSOLE("               \t\tto. Default is the input filepath with \"gfxr\" replaced");
    GFXRECON_WRITE_CONSOLE("               \t\tby \"jsonl\".");
#if defined(WIN32)
    GFXRECON_WRITE_CONSOLE("  --api <api>\t\tDump ASCII output for the specified API (Windows only).");
    GFXRECON_WRITE_CONSOLE("          \t\tAvailable values are:");
    GFXRECON_WRITE_CONSOLE("          \t\t    %s\tDump only Vulkan API calls.", kApiFamilyVulkan);
    GFXRECON_WRITE_CONSOLE("          \t\t    %s\tDump only D3D12 API calls.", kApiFamilyD3D12);
    GFXRECON_WRITE_CONSOLE("          \t\t    %s\t\tDump both Vulkan and D3D12 API calls. This is the default.",
                           kApiFamilyAll);
#if defined(WIN32) && defined(_DEBUG)
    GFXRECON_WRITE_CONSOLE("  --no-debug-popup\tDisable the 'Abort, Retry, Ignore' message box");
    GFXRECON_WRITE_CONSOLE("                  \tdisplayed when abort() is called (Windows debug only).");
#endif
#endif
}

namespace
{

std::string GetOutputFileName(const gfxrecon::util::ArgumentParser& arg_parser, const std::string& input_filename)
{
    std::string output_filename;
    if (arg_parser.IsArgumentSet(kOutput))
    {
        output_filename = arg_parser.GetArgumentValue(kOutput);
    }
    else
    {
        output_filename   = input_filename;
        size_t suffix_pos = output_filename.find(GFXRECON_FILE_EXTENSION);
        if (suffix_pos != std::string::npos)
        {
            output_filename = output_filename.substr(0, suffix_pos);
        }
        output_filename += ".jsonl";
    }
    return output_filename;
}

} // namespace

int main(int argc, const char** argv)
{
    gfxrecon::util::Log::Init();

    gfxrecon::util::ArgumentParser arg_parser(argc, argv, kOptions, kArguments);

    if (CheckOptionPrintUsage(argv[0], arg_parser) || CheckOptionPrintVersion(argv[0], arg_parser))
    {
        gfxrecon::util::Log::Release();
        exit(0);
    }

    if (arg_parser.IsInvalid() || (arg_parser.GetPositionalArgumentsCount() != 1))
    {
        PrintUsage(argv[0]);
        gfxrecon::util::Log::Release();
        exit(-1);
    }

    if (arg_parser.IsArgumentSet(kOutput) && arg_parser.GetArgumentValue(kOutput).empty())
    {
        GFXRECON_LOG_ERROR("Empty string given for argument \"--output\"; must be a valid path or 'stdout'");
        gfxrecon::util::Log::Release();
        exit(-1);
    }

#if defined(WIN32) && defined(_DEBUG)
    if (arg_parser.IsOptionSet(kNoDebugPopup))
    {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    }
#endif

    const auto&       positional_arguments = arg_parser.GetPositionalArguments();
    const std::string input_filename       = positional_arguments[0];
    const std::string output_filename      = GetOutputFileName(arg_parser, input_filename);

    gfxrecon::decode::FileProcessor file_processor;
    if (file_processor.Initialize(input_filename))
    {
        FILE* output_file = nullptr;
        if (gfxrecon::util::platform::StringCompare(output_filename.c_str(), "stdout") == 0)
        {
            output_file = stdout;
        }
        else
        {
            gfxrecon::util::platform::FileOpen(&output_file, output_filename.c_str(), "w");
        }

        if (output_file)
        {
            auto to_string_flags = gfxrecon::util::kToString_Default;
            if (arg_parser.IsOptionSet(kJsonObjectArgument))
            {
                to_string_flags = gfxrecon::util::kToString_Formatted;
            }

            gfxrecon::decode::VulkanAsciiConsumer vulkan_ascii_consumer;
            gfxrecon::decode::VulkanDecoder       vulkan_decoder;

#if defined(WIN32)
            gfxrecon::decode::Dx12AsciiConsumer dx12_ascii_consumer;
            gfxrecon::decode::Dx12Decoder       dx12_decoder;
#endif

            gfxrecon::decode::VulkanTrackedObjectInfoTable tracked_object_info_table;
            auto vulkan_replay_options = GetVulkanReplayOptions(arg_parser, input_filename, &tracked_object_info_table);
            if (vulkan_replay_options.enable_vulkan)
            {
                vulkan_ascii_consumer.Initialize(output_file,
                                                 GFXRECON_PROJECT_VERSION_STRING,
                                                 (std::to_string(VK_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                                  std::to_string(VK_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE)) + "." +
                                                  std::to_string(VK_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE)))
                                                     .c_str(),
                                                 input_filename.c_str());
                vulkan_decoder.AddConsumer(&vulkan_ascii_consumer);
                file_processor.AddDecoder(&vulkan_decoder);
            }

#if defined(WIN32)
            auto dx_replay_options = GetDxReplayOptions(arg_parser);
            if (dx_replay_options.enable_d3d12)
            {
                dx12_ascii_consumer.Initialize(output_file, to_string_flags);
                dx12_decoder.AddConsumer(&dx12_ascii_consumer);
                file_processor.AddDecoder(&dx12_decoder);
            }
#endif

            file_processor.ProcessAllFrames();

            if (vulkan_replay_options.enable_vulkan)
            {
                vulkan_ascii_consumer.Destroy();
            }

#if defined(WIN32)
            if (dx_replay_options.enable_d3d12)
            {
                dx12_ascii_consumer.Destroy();
            }
#endif

            if (output_file != stdout)
            {
                gfxrecon::util::platform::FileClose(output_file);
            }
        }
        else
        {
            GFXRECON_LOG_ERROR("Failed to open/create output file \"%s\"; is the path valid?", output_filename.c_str());
        }
    }

    gfxrecon::util::Log::Release();
    return 0;
}
