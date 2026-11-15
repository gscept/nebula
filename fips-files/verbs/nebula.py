"""nebula project paths

Nothing here is stored. The project root is the directory the project info file
lives in, which for fips is always the project directory, and the toolkit root is
the nebula checkout fips imports. Both can be overridden with the same environment
variables the C++ resolver reads (NEBULA_PROJECT / NEBULA_TOOLKIT), so the verb and
the toolkit tools agree.

fips nebula
fips nebula [work|toolkit]
fips nebula cleannidl
"""

from mod import log, util, settings
import os
import shutil

keys = ['work', 'toolkit']

# the environment variables IO::ProjectLocator reads for the same keys
env_keys = { 'work': 'NEBULA_PROJECT', 'toolkit': 'NEBULA_TOOLKIT' }

#-------------------------------------------------------------------------------
def resolve(fips_dir, proj_dir, key) :
    """resolve a nebula key to (value, source)

    :param fips_dir:    absolute path to fips
    :param proj_dir:    absolute path to the project directory
    :param key:         'work' or 'toolkit'
    :returns:           (absolute path, where the value came from), path is
                        None if the key cannot be resolved
    """
    env = os.environ.get(env_keys[key])
    if env :
        return os.path.abspath(env), 'environment'

    if key == 'work' :
        # the project info file lives in the project root, so the project
        # directory *is* the project root
        return proj_dir, 'project directory'

    # the toolkit root is the nebula checkout fips imports
    import_dir = util.get_project_dir(fips_dir, 'nebula')
    if os.path.isdir(import_dir) :
        return import_dir, 'fips import'
    return None, 'not resolved'

#-------------------------------------------------------------------------------
def run(fips_dir, proj_dir, args) :
    """run the 'nebula' verb"""
    if len(args) > 0 :
        noun = args[0]
        if noun in keys :
            value, source = resolve(fips_dir, proj_dir, noun)
            if value is None :
                log.error("'{}' could not be resolved".format(noun))
            log.info('{} ({})'.format(value, source))
        elif noun == 'cleannidl' :
            proj = util.get_project_name_from_dir(proj_dir)
            cfg = settings.get(proj_dir, 'config')
            path = util.get_build_dir(fips_dir, proj, cfg) + '/nidl'
            shutil.rmtree(path, True)
        else :
            log.error("Unknown command '{}'".format(noun))
    else :
        log.colored(log.YELLOW, "Current paths")
        for key in keys :
            value, source = resolve(fips_dir, proj_dir, key)
            log.optional(key, '{} ({})'.format(value if value else '(not resolved)', source))

#-------------------------------------------------------------------------------
def help() :
    """print 'nebula' help"""
    log.info(log.YELLOW +
             "fips nebula\n"
             "  prints the resolved paths and where each came from\n"
             "fips nebula [work|toolkit]\n"
             "  work     -- project root, the directory the project info file is in\n"
             "  toolkit  -- nebula root, the fips import\n"
             "fips nebula cleannidl\n"
             "  cleans all nidl files which forces a regeneration upon compilation\n")
