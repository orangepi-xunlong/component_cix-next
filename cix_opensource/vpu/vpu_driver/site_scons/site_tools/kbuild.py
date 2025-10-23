import os

def sourceToObject(source):
    name, ext = os.path.splitext(source)
    if ext == '.c':
        name += '.o'
    else:
        raise Exception('Error: Unknown source file "' + source + '".')

    return name

def kbuildSharedLibraryAction(env, target, source):
    for t in env.Flatten(target):
        kbuild = 'obj-m += %s.o\n' % (str(t))
        kbuild += '%s-objs :=' % (str(t))
        for s in env.Flatten(source):
            kbuild += ' %s' % (sourceToObject(str(s)))
        kbuild += '\n'

        t.write(kbuild)

def kbuildSharedLibraryEmitter(env, target, source):
    targets = []
    for t in env.Flatten(target):
        value = env.Value(str(t))
        env.AlwaysBuild(value)
        targets.append(value)

    sources = []
    for s in source:
        path = os.path.relpath(s.srcnode().abspath, env.Dir('.').srcnode().abspath)
        value = env.Value(path)
        sources.append(value)

    return targets, sources

def kbuildAction(target, source, env):
    for t in env.Flatten(target):
        with open(str(t), 'w') as f:
            ccflags = []
            if 'CCFLAGS' in env:
                ccflags += env.Flatten(env['CCFLAGS'])
            if 'CPPDEFINES' in env:
                ccflags += ['-D' + c for c in env.Flatten(env['CPPDEFINES'])]
            if ccflags:
                f.write('ccflags-y += %s\n\n' % ' '.join(ccflags))

            for s in env.Flatten(source):
                f.write('%s\n' % (s.read()))

def exists(env):
    return True

def generate(env):
    env['BUILDERS']['SharedLibrary'] = env.Builder(action = kbuildSharedLibraryAction, emitter = kbuildSharedLibraryEmitter)
    env.Append(BUILDERS = {'KBuild': env.Builder(action = kbuildAction)})
