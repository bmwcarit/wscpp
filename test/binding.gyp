{
  "target_defaults":
    {
        "cflags_cc" :
        [
            "-Wall",
            "-Wextra",
            "-Wno-unused-parameter",
        ],
        "cflags!": [ "-fno-exceptions"],
        "cflags_cc!": [ "-fno-exceptions"],
        "include_dirs":
        [
            "../src",
            "<!(node -e \"require('nan')\")"
        ],
        'conditions': [
            ['OS=="mac"',
                {
                    'xcode_settings': {
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                        'OTHER_CFLAGS' : ['-std=c++14']
                    },
                    'libraries': ['-L/usr/local/lib'],
                    'include_dirs': ['/usr/local/include']
                }
            ]
          ],
    },
  "targets":
  [
    {
        "target_name" : "marshallingTest",
        "sources"     : [ "cpp/marshalling.test.cpp" ],
        # needs to be specified per target because node-gyp adds "-std=gnu++0x"
        "cflags_cc"   : [ "-std=c++14" ],
    }
  ]
}
