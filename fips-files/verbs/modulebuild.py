"""build specific module target through fips

modulebuild build <target> [config] [-- build tool args]
"""

from mod import log, settings, project


def run(fips_dir, proj_dir, args):
    """run the 'modulebuild' verb"""
    if '--' in args:
        idx = args.index('--')
        build_tool_args = args[(idx + 1):]
        args = args[:idx]
    else:
        build_tool_args = None

    if len(args) < 2:
        help()
        log.error("expected: modulebuild build <target> [config]")

    noun = args[0]
    if noun != 'build':
        help()
        log.error("unknown command '{}', expected 'build'".format(noun))

    target = args[1]
    if not target:
        log.error("target must not be empty")

    cfg_name = args[2] if len(args) > 2 else settings.get(proj_dir, 'config')

    if not project.build(fips_dir, proj_dir, cfg_name, target, build_tool_args):
        log.error("failed to build target '{}'".format(target))


def help():
    """print modulebuild help"""
    log.info(log.YELLOW +
             "fips modulebuild build <target> [config] [-- build tool args]\n" + log.DEF +
             "    build a specific CMake target in the selected config\n" +
             "    if config is omitted, current fips setting is used")
