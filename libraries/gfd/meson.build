subdir('src')

libnnsgfd_includes = include_directories('include')

libnnsgfd = library('nnsgfd',
    dependencies: nitrosdk_dep,
    include_directories: [nitrosystem_includes, libnnsgfd_includes],
    pic: false,
    sources: libnnsgfd_srcs
)

nitrosystem_libs += libnnsgfd
