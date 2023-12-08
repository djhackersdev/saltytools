project('saltytools', 'c', version: '0.1.0')

libpng_dep = dependency('libpng', fallback: ['libpng', 'libpng_dep'])
openssl_dep = dependency('openssl', fallback: ['openssl', 'openssl_dep'])

inc = include_directories('.')

subdir('573file')
subdir('picture')
subdir('util')

subdir('ifsdump')
subdir('xmldump')
