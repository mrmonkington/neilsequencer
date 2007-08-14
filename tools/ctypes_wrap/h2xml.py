"""h2xml - convert C include file(s) into an xml file by running gccxml."""
import sys, os, tempfile, re, ConfigParser
import cparser
from optparse import OptionParser

def main(argv=None):
    if argv is None:
        argv = sys.argv

    def add_option(option, opt, value, parser):
        parser.values.gccxml_options.extend((opt, value))

    # Hm, should there be a way to disable the config file?
    # And then, this should be done AFTER the parameters are processed.
    config = ConfigParser.ConfigParser()
    try:
        config.read("h2xml.cfg")
    except ConfigParser.ParsingError, detail:
        print >> sys.stderr, detail
        return 1

    def get_option(option, default_value):
        # return an option from the platform specific section of the
        # config file, or return the default_value if either the
        # section or the option is not present.
        try:
            return config.get(sys.platform, option)
        except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
            return default_value

    excluded = get_option("excluded", "").split()
    excluded_re = get_option("excluded_re", "").split()
    
    parser = OptionParser("usage: %prog includefile ... [options]")
    parser.add_option("-q", "--quiet",
                      dest="quiet",
                      action="store_true",
                      default=False)

    parser.add_option("-D",
                      type="string",
                      action="callback",
                      callback=add_option,
                      dest="gccxml_options",
                      help="macros to define",
                      metavar="NAME[=VALUE]",
                      default=[])

    parser.add_option("-U",
                      type="string",
                      action="callback",
                      callback=add_option,
                      help="macros to undefine",
                      metavar="NAME")

    parser.add_option("-I",
                      type="string",
                      action="callback",
                      callback=add_option,
                      dest="gccxml_options",
                      help="additional include directories",
                      metavar="DIRECTORY")

    parser.add_option("-o",
                      dest="xmlfile",
                      help="XML output filename",
                      default=None)

    parser.add_option("-c", "--cpp-symbols",
                      dest="cpp_symbols",
                      action="store_true",
                      help="try to find #define symbols - this may give compiler errors, " \
                      "so it's off by default.",
                      default=False)

    parser.add_option("-k",
                      dest="keep_temporary_files",
                      action="store_true",
                      help="don't delete the temporary files created "\
                      "(useful for finding problems)",
                      default=False)

    parser.add_option("-s",
                      dest="excluded_symbols",
                      action="append",
                      help="specify preprocessor symbol name to exclude",
                      default=excluded)

    parser.add_option("-r",
                      dest="excluded_symbols_re",
                      action="append",
                      help="regular expression for preprocessor symbol names to exclude",
                      default=[])

    options, files = parser.parse_args(argv[1:])

    if not files:
        print "Error: no files to process"
        print >> sys.stderr, __doc__
        return 1

    options.flags = options.gccxml_options

    options.verbose = not options.quiet

    options.excluded_symbols_re = [re.compile(pat) for pat in options.excluded_symbols_re]

    try:
        parser = cparser.IncludeParser(options)
        parser.parse(files)
    except cparser.CompilerError, detail:
        print >> sys.stderr, "CompilerError:", detail
        return 1

if __name__ == "__main__":
    sys.exit(main())
