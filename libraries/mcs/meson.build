subdir('src')

libnnsmcs_includes = include_directories('include')

libnnsmcs = library('nnsmcs',
    dependencies: nitrosdk_dep,
    include_directories: [nitrosystem_includes, libnnsmcs_includes],
    pic: false,
    sources: libnnsmcs_srcs
)

nitrosystem_libs += libnnsmcs
