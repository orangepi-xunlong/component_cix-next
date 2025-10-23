import os

def BuildRecursive(env, include = [], exclude = []):
    '''
    Searches immediate subdirectories 'sconstruct' files and calls
    SConstruct one them.
    '''

    cwd = env.Dir('.').srcnode().abspath

    for dir in sorted(os.listdir(cwd) + include):
        if dir not in exclude:
            sconscript = os.path.join(cwd, os.path.join(dir, 'sconscript'))
            if os.path.isfile(sconscript):
                variant_dir = os.path.relpath(env.Dir(dir).srcnode().abspath, env.Dir('#').abspath)
                variant_dir = os.path.join(env['BUILD_DIR_PATH'], variant_dir)
                env.SConscript(sconscript, variant_dir = variant_dir, duplicate = False)

def exists(env):
    return True

def generate(env):
    env.AddMethod(BuildRecursive, 'BuildRecursive')
