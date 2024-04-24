// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <vector>

#include <toyos/util/algorithm.hpp>
#include <toyos/util/string.hpp>
#include <toyos/util/trace.hpp>

#include <optionparser/optionparser.h>

namespace cmdline
{

    /**
     * Glue for the optionparser library.
     */
    namespace optionparser
    {
        constexpr char DISABLED_TESTCASES_DELIMITER = ',';

        /**
         * Index into the `usage` array.
         */
        enum option_index
        {
            SERIAL,
            XHCI,
            XHCI_POWER,
            DISABLED_TESTCASES,
        };

        /**
         * Cmdline parameter description for the option parser library.
         * The reference description is in the README. The help texts here are
         * ignored.
         */
        static constexpr const option::Descriptor usage[] = {
            // index, type, shorthand, name, checkarg, help
            { SERIAL, 0, "", "serial", option::Arg::Optional, "" },
            { XHCI, 0, "", "xhci", option::Arg::Optional, "" },
            { XHCI_POWER, 0, "", "xhci-power", option::Arg::Optional, "" },
            { DISABLED_TESTCASES, 0, "", "disable-testcases", option::Arg::Optional, "" },

            { 0, 0, nullptr, nullptr, nullptr, nullptr }
        };
    };  // namespace optionparser

    /**
     * Wrapper class around the optionparser library.
     */
    class cmdline_parser
    {
     public:
        /**
         * Instantiates an instance with the given commandline and available options.
         *
         * Takes the string, tokenizes it using a space as separator, and creates
         * an argument vector (argc/argv pair).
         *
         * \param cmdline The command line as string.
         */
        explicit cmdline_parser(const std::string& cmdline)
            : arguments(tokenize(cmdline, ' '))
        {
            auto usage = cmdline::optionparser::usage;

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
         * Returns something if the serial cmdline modifier (flag or option) is
         * present.
         *
         * If the modifier was provided as flag, the inner string is empty.
         */
        std::optional<std::string> serial_option()
        {
            return option_value(optionparser::option_index::SERIAL);
        }

        /**
         * Returns something if the xhci cmdline modifier (flag or option) is
         * present.
         *
         * If the modifier was provided as flag, the inner string is empty.
         */
        std::optional<std::string> xhci_option()
        {
            return option_value(optionparser::option_index::XHCI);
        }

        /**
         * Returns the specified xhci-power cmdline modifier or the default.
         */
        std::string xhci_power_option()
        {
            return option_value(optionparser::option_index::XHCI_POWER).value_or("0");
        }

        /**
         * Returns the disable-testcases cmdline modifier or the default.
         */
        std::vector<std::string> disable_testcases_option()
        {
            auto disabled_testcases_str = option_value(optionparser::option_index::DISABLED_TESTCASES).value_or("");
            return util::string::split(disabled_testcases_str, cmdline::optionparser::DISABLED_TESTCASES_DELIMITER);
        }

     private:
        std::vector<std::string> arguments;
        std::vector<const char*> argv;
        option::Stats stats;
        std::vector<option::Option> options, buffer;
        option::Parser parser;

        /**
         * Returns something if the given cmdline modifier (flag or option) is
         * present.
         *
         * If the modifier was provided as flag, the inner string is empty.
         *
         * \param idx The index of the option in the original usage vector.
         * \return An optional string, only valid if an option was specified.
         *         The string is the empty string if no argument was specified.
         */
        [[nodiscard]] std::optional<std::string> option_value(size_t idx) const
        {
            const auto& o = options.at(idx);
            if (o) {
                return { o.arg ?: "" };
            }
            return {};
        }
    };
}  // namespace cmdline
