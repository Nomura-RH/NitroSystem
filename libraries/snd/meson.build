subdir('src')

libnnssnd = library('nnssnd',
    dependencies: nitrosdk_dep,
    include_directories: nitrosystem_includes,
    pic: false,
    sources: libnnssnd_srcs
)

nitrosystem_libs += libnnssnd
