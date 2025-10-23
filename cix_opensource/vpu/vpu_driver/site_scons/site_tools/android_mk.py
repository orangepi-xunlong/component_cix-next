import filecmp
import os
import SCons.Node.FS

def CommonEmitter(env, target, source):
    # Convert target file nodes to value nodes.
    targets = []
    for t in env.Flatten(target):
        value = env.Value(t.name)
        value.type = type
        value.name = t.name
        env.AlwaysBuild(value)
        targets.append(value)

    # Append target as source to 'Android.mk' target.
    env['ANDROID_MK'].data[0].executor.batches[0].sources.extend(targets)

    # Convert source file nodes to value nodes.
    sources = []
    for s in source:
        if isinstance(s, SCons.Node.FS.File):
            s = env.Value(s.srcnode().path)
            s.type = 'source'
            s.name = str(s)

        sources.append(s)

    return targets, sources

def ProgramEmitter(env, target, source):
    target, source = CommonEmitter(env, target, source)

    for t in target:
        t.type = 'program'

    return target, source

def StaticLibraryEmitter(env, target, source):
    target, source = CommonEmitter(env, target, source)

    for t in target:
        t.type = 'static_lib'
        t.value = 'lib' + t.value

    return target, source

def SharedLibraryEmitter(env, target, source):
    target, source = CommonEmitter(env, target, source)

    for t in target:
        t.type = 'shared_lib'
        t.value = 'lib' + t.value

    return target, source

def ObjectEmitter(env, target, source):
    sources = []
    for s in env.Flatten(source):
        v = env.Value(s.srcnode().path)
        v.type = 'source'
        sources.append(v)

    return sources, []

def CommonAction(target, source, env, rule):
    # Add target to link binary.
    v = '\ninclude $(CLEAR_VARS)\n'
    v += 'LOCAL_MODULE := %s\n' % str(target)

    # Add source dependencies.
    source_files = []
    static_libraries = []
    shared_libraries = []
    for s in env.Flatten(source):
        if s.type == 'source':
            source_files.append(s)
        elif s.type == 'static_lib':
            static_libraries.append(s)
        elif s.type == 'shared_lib':
            shared_libraries.append(s)
        else:
            raise Exception('Unknown source file: ' + s)

    if 'LIBS' in env:
        static_libraries += ['lib' + l for l in env.Flatten(env['LIBS'])]

    if 'SLIBS' in env:
        shared_libraries += ['lib' + l for l in env.Flatten(env['SLIBS'])]

    if source_files:
        v += 'LOCAL_SRC_FILES := %s\n' % ' '.join(map(str, source_files))

    if static_libraries:
        v += 'LOCAL_STATIC_LIBRARIES := %s\n' % ' '.join(static_libraries)

    if shared_libraries:
        v += 'LOCAL_SHARED_LIBRARIES := %s\n' % ' '.join(shared_libraries)

    if 'LINKFLAGS' in env:
        v += 'LOCAL_LDFLAGS := %s\n' % ' '.join(env['LINKFLAGS'])

    if 'CPPPATH' in env:
        v += 'LOCAL_C_INCLUDES := %s\n' % ' '.join([c.replace('#', '$(LOCAL_PATH)/').replace('//', '/') for c in env.Flatten(env['CPPPATH'])])

    # Add cflags.
    ccflags = []
    if 'CCFLAGS' in env:
        ccflags += env.Flatten(env['CCFLAGS'])
    if 'CPPFLAGS' in env:
        ccflags += env.Flatten(env['CPPFLAGS'])
    if 'CPPDEFINES' in env:
        ccflags += ['-D' + c for c in env.Flatten(env['CPPDEFINES'])]
    if ccflags:
        v += 'LOCAL_CFLAGS := %s\n' % ' '.join(ccflags)

    if 'CXXFLAGS' in env:
        v += 'LOCAL_CPPFLAGS := %s\n' % ' '.join(env['CXXFLAGS'])

    # Add rule to build program.
    v += 'include $(%s)' % rule

    return v

def Program(target, source, env):
    for t in target:
        v = CommonAction(t, source, env, 'BUILD_EXECUTABLE')

        # Write string to target Value node.
        t.write(v)

def StaticLibrary(target, source, env):
    for t in target:
        v = CommonAction(t, source, env, 'BUILD_STATIC_LIBRARY')

        # Write string to target Value node.
        t.write(v)

def SharedLibrary(target, source, env):
    for t in target:
        v = CommonAction(t, source, env, 'BUILD_SHARED_LIBRARY')

        # Write string to target Value node.
        t.write(v)

def Object(target, source, env):
    print("Object(%s, %s)" % (str(target), str(source)))

def AndroidMk(target, source, env):
    for t in target:
        t = str(t)

        with open(t, 'w') as f:
            f.write('LOCAL_PATH := $(call my-dir)\n')

            for s in env.Flatten(source):
                f.write('%s\n' % (s.read()))

        android_mk = os.path.splitext(t)[0] + '.mk'
        if not os.path.exists(android_mk) or not filecmp.cmp(t, android_mk):
            os.rename(t, android_mk)
        else:
            os.remove(t)


def Install(env, target, source):
    for t in map(str, env.Flatten(target)):
        for s in map(str, env.Flatten(source)):
            sfile = env.File(s)
            tfile = env.File(os.path.join(t, os.path.basename(s)))

            if sfile.srcnode().exists():
                env.CopyAs(tfile, sfile)

def exists(env):
    return True

def generate(env):
    env['BUILDERS']['Program'] = env.Builder(action = Program, emitter = ProgramEmitter)
    env['BUILDERS']['StaticLibrary'] = env.Builder(action = StaticLibrary, emitter = StaticLibraryEmitter)
    env['BUILDERS']['SharedLibrary'] = env.Builder(action = SharedLibrary, emitter = SharedLibraryEmitter)
    env['BUILDERS']['Object'] = env.Builder(action = Object, emitter = ObjectEmitter)
    env['BUILDERS']['Install'] = Install
    env.Append(BUILDERS = {'AndroidMk': env.Builder(action = AndroidMk, suffix='.tmp')})

    if not 'ANDROID_MK' in env:
        build_android_mk = env.AndroidMk('#Android', None)
        env.AlwaysBuild(build_android_mk)
        env.Append(ANDROID_MK = build_android_mk)
