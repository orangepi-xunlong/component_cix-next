import os
import SCons.Node.FS

def ninjaProgramAction(target, source, env):
    for t in target:
        # Add target to link binary.
        v = 'build %s: link' % str(t)

        # Add source dependencies.
        for s in env.Flatten(source):
            v += ' %s' % (str(s))
        v += '\n'

        # Add libraries.
        if 'LIBS' in env:
            v += '  libs ='
            for l in env['LIBS']:
                v += ' -l' + l
            v += '\n'

        # Write string to target Value node.
        t.write(v)

def ninjaStaticLibraryAction(target, source, env):
    for t in env.Flatten(target):
        # Add build target to create library.
        v = 'build %s: static_lib' % (str(t))
        
        # Add source dependencies.
        for s in env.Flatten(source):
            v += ' %s' % (str(s))
        v += '\n'

        # Write string to target Value node.
        t.write(v)

def ninjaStaticLibraryEmitter(env, target, source):
    target, source = ninjaEmitter(env, target, source)

    # Rename static libraries to lib<name>.a.
    for t in env.Flatten(target):
        (d, f) = os.path.split(t.value)
        t.value = os.path.join(d, 'lib' + f + '.a')

    return target, source

def ninjaSharedLibraryAction(target, source, env):
    for t in env.Flatten(target):
        # Add build target to create library.
        v = 'build %s: shared_lib' % (str(t))
        
        # Add source dependencies.
        for s in env.Flatten(source):
            v += ' %s' % (str(s))
        v += '\n'

        # Write string to target Value node.
        t.write(v)

def ninjaSharedLibraryEmitter(env, target, source):
    target, source = ninjaEmitter(env, target, source)

    # Rename shared libraries to lib<name>.so.
    for t in env.Flatten(target):
        (d, f) = os.path.split(t.value)
        t.value = os.path.join(d, 'lib' + f + '.so')

    return target, source

def ninjaObjectAction(target, source, env):
    for t in env.Flatten(target):
        # Add build target to create object.
        v = 'build %s: cc' % (str(t))

        # Add source dependencies.
        for s in env.Flatten(source):
            v += ' %s' % (str(s))
        v += '\n'

        # Write string to target Value node.
        t.write(v)

def ninjaObjectEmitter(env, target, source):
    # Convert target file nodes to value nodes.
    targets = []
    for t in env.Flatten(target):
        t = env.Value(t.tpath)
        targets.append(t)
        env.AlwaysBuild(t)

    # Append target as source to 'build.ninja' target.
    env['BUILD_NINJA'].data[0].executor.batches[0].sources.extend(targets)

    # Convert source file nodes to value nodes.
    sources = []    
    for s in env.Flatten(source):
        s = env.Value(s.srcnode().path)
        sources.append(s)
        env.AlwaysBuild(s)
    
    return targets, sources
    
def ninjaEmitter(env, target, source):
    # Convert target file nodes to value nodes.
    targets = []
    for t in env.Flatten(target):
        value = env.Value(t.tpath)
        env.AlwaysBuild(value)
        targets.append(value)

    # Append target as source to 'build.ninja' target.
    env['BUILD_NINJA'].data[0].executor.batches[0].sources.extend(targets)

    # Convert source file nodes to value nodes.
    sources = []
    for s in source:
        if isinstance(s, SCons.Node.FS.File):
            name, ext = os.path.splitext(s.name)

            # If file is a known source file, then create an object target.
            if ext in ('.c', '.cpp'):
                s = env.Object(name + '.o', str(s))
            # Else create a value target and hope the file can be linked as is.
            else:
                s = env.Value(s.path)

        sources.append(s)

    return targets, sources

def ninjaAction(target, source, env):
    for t in target:
        with open(str(t), 'w') as f:
            f.write('builddir = %s\n\n' % (env['BUILD_DIR_PATH']))
            
            ccflags = []
            if 'CCFLAGS' in env:
                ccflags += env.Flatten(env['CCFLAGS'])
            if 'CPPDEFINES' in env:
                ccflags += ['-D' + c for c in env.Flatten(env['CPPDEFINES'])]
            if ccflags:
                f.write('cflags += %s\n\n' % ' '.join(ccflags))

            f.write('rule cc\n')
            f.write('  depfile = $out.d\n')
            f.write('  command = %s -MMD -MF $out.d $cflags -o $out -c $in\n' % env['CC'])
            f.write('rule link\n')
            f.write('  command = %s $cflags $ldflags -o $out $in $libs\n' % env['CC'])
            f.write('rule static_lib\n')
            f.write('  command = %s rcs $out $in\n' % env['AR'])
            f.write('rule shared_lib\n')
            f.write('  command = %s $cflags -o $out -shared $in $libs\n' % env['CC'])
            f.write('\n')

            for s in env.Flatten(source):
                f.write('%s\n' % (s.read()))

def exists(env):
    return True

def generate(env):
    env['BUILDERS']['Program'] = env.Builder(action = ninjaProgramAction, emitter = ninjaEmitter)
    env['BUILDERS']['StaticLibrary'] = env.Builder(action = ninjaStaticLibraryAction, emitter = ninjaStaticLibraryEmitter)
    env['BUILDERS']['SharedLibrary'] = env.Builder(action = ninjaSharedLibraryAction, emitter = ninjaSharedLibraryEmitter)
    env['BUILDERS']['Object'] = env.Builder(action = ninjaObjectAction, emitter = ninjaObjectEmitter)
    env.Append(BUILDERS = {'Ninja': env.Builder(action = ninjaAction)})

    if not 'BUILD_NINJA' in env:
        build_ninja = env.Ninja('#build.ninja', None)
        env.AlwaysBuild(build_ninja)
        env.Append(BUILD_NINJA = build_ninja)

