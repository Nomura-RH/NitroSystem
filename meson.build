project('NitroSystem', 'c',
    default_options: {
        'default_library': 'static',
        'prefer_static':    true
    },
    meson_version: '>=1.2.0',
    version: '071126.1'
)

buildtype = get_option('buildtype')

############################################################
###                    COMPILER FLAGS                    ###
############################################################

if not meson.is_subproject() and buildtype != 'plain'
c_args = [
    '-O4,p',
    '-proc', 'arm946e',
    '-enum', 'int',
    '-lang', 'c99',
    '-Cpp_exceptions', 'off',
    '-gccext,on',
    '-msgstyle', 'gcc',
    '-ipa', 'file',
    '-interworking',
    '-inline', 'on,noauto',
    '-char', 'signed',
    '-nosyspath',
    '-stdinc'
]
else
c_args = []
endif

mwrap_ver = get_option('mwrap_ver')
if mwrap_ver != 'default'
    c_args += ['-wrap:ver', mwrap_ver]
endif

add_project_arguments(c_args,
    language: 'c',
    native: false
)

nitrosystem_args = [
    '-DSDK_CW_FORCE_EXPORT_SUPPORT',
    '-DSDK_TS',
    '-DSDK_4M',
    '-DSDK_ARM9',
    '-DSDK_CW',
    '-DSDK_FINALROM',
    '-DSDK_CODE_ARM',
    '-DNNS_FINALROM'
]

add_project_arguments(nitrosystem_args,
    language: 'c',
    native: false
)

############################################################
###                    INCLUDE PATHS                     ###
############################################################
nitrosystem_includes = include_directories('include')

############################################################
###                     DEPENDENCIES                     ###
############################################################
nitrosdk_dep = dependency('NitroSDK')

############################################################
###                      LIBRARIES                       ###
############################################################
subdir('libraries')

############################################################
###                    FOR CONSUMERS                     ###
############################################################
nitrosystem_dep = declare_dependency(
    compile_args: nitrosystem_args,
    include_directories: nitrosystem_includes,
    link_with: nitrosystem_libs
)

meson.override_dependency('NitroSystem', nitrosystem_dep)
