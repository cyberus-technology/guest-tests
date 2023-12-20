/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <optional>
#include <string>
#include <vector>

#include <toyos/optionparser.h>
#include <toyos/util/algorithm.hpp>

namespace cmdline
{

    /**
     * Glue for the optionparser library.
     */
    namespace optionparser
    {
        /**
         * Index into the `usage` array.
         */
        enum option_index
        {
            SERIAL,
            XHCI,
            XHCI_POWER
        };

        static constexpr const option::Descriptor usage[] = {
            // index, type, shorthand, name, checkarg, help
            { SERIAL, 0, "", "serial", option::Arg::Optional, "Enable serial console, with an optional port <port> in hex." },
            { XHCI, 0, "", "xhci", option::Arg::Optional, "Enable xHCI debug console, with optional custom serial number." },
            { XHCI_POWER, 0, "", "xhci-power", option::Arg::Optional, "Set the USB power cycle method (0=nothing, 1=powercycle)." },

            { 0, 0, 0, 0, 0, 0 }
        };
    };  // namespace optionparser

    /**
     * Wrapper class around the optionparser library.
     */
    class cmdline_parser
    {
     public:
        cmdline_parser() = default;

        /**
         * Instantiates an instance with the given commandline and available options.
         *
         * Takes the string, tokenizes it using a space as separator, and creates
         * an argument vector (argc/argv pair).
         *
         * \param cmdline The command line as string.
         * \param usage   An array of option descriptors.
         */
        cmdline_parser(const std::string& cmdline,
                       const option::Descriptor usage[])
            : arguments(tokenize(cmdline, ' '))
        {
            // Copy the tokenized arguments into the member variable,
            // because the raw string pointers will be used by the parser
            for (const auto& s : arguments) {
                if (s.find("--") != std::string::npos) {
                    argv.push_back(s.c_str());
                }
            }

            stats = option::Stats(usage, argv.size(), argv.data());

            options.resize(stats.options_max);
            buffer.resize(stats.buffer_max);

            parser = option::Parser(usage, argv.size(), argv.data(), options.data(), buffer.data());
            PANIC_ON(parser.error(), "Argument parsing failed!");
        }

        /**
         * Retrieves an option value for a given index.
         *
         * \param idx The index of the option in the original usage vector.
         * \return An optional string, only valid if an option was specified.
         *         The string is the empty string if no argument was specified.
         */
        std::optional<std::string> option_value(size_t idx) const
        {
            const auto& o = options.at(idx);
            if (o) {
                return { o.arg ?: "" };
            }
            return {};
        }

     private:
        std::vector<std::string> arguments;
        std::vector<const char*> argv;
        option::Stats stats;
        std::vector<option::Option> options, buffer;
        option::Parser parser;
    };
}  // namespace cmdline
