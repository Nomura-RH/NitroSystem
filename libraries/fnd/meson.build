subdir('src')

libnnsfnd_includes = include_directories('include')

libnnsfnd = library('nnsfnd',
    dependencies: nitrosdk_dep,
    include_directories: [nitrosystem_includes, libnnsfnd_includes],
    pic: false,
    sources: libnnsfnd_srcs
)

nitrosystem_libs += libnnsfnd
