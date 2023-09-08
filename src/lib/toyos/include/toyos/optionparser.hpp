/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "optional"
#include "string"
#include "vector"

#include "optionparser.h"

#include "toyos/util/algorithm.hpp"

/**
 * Wrapper class around the optionparser library.
 */
class optionparser
{
public:
   optionparser() = default;

   /**
     * Instantiates an instance with the given commandline and available options.
     *
     * Takes the string, tokenizes it using a space as separator, and creates
     * an argument vector (argc/argv pair).
     *
     * \param cmdline The command line as string.
     * \param usage   An array of option descriptors.
     */
   optionparser(const std::string& cmdline, const option::Descriptor usage[])
      : arguments(tokenize(cmdline, ' '))
   {
      // Copy the tokenized arguments into the member variable,
      // because the raw string pointers will be used by the parser
      for(const auto& s : arguments) {
         if(s.find("--") != std::string::npos) {
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
      if(o) {
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
