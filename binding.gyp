{
  'targets': [
  {
    'target_name': 'wscpp-client',
    'sources': [ 'src/wscpp-client.cpp' ],
    'cflags_cc': [ '-std=c++14' ],
    'cflags!': [ '-fno-exceptions'],
    'cflags_cc!': [ '-fno-exceptions', '-fno-rtti'],
    'include_dirs': [
        "<!(node -e \"require('nan')\")",
        "node_modules/websocketpp",
        "node_modules/concurrentqueue"
    ],
    'libraries': ['-lboost_system'],
    'conditions': [
        ['OS=="mac"',
            {
                'xcode_settings': {
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                    'GCC_ENABLE_CPP_RTTI': 'YES',
                    'GCC_OPTIMIZATION_LEVEL': '3',
                    'OTHER_CFLAGS' : ['-std=c++14']
                },
                'libraries': ['-L/usr/local/lib'],
                'include_dirs': ['/usr/local/include']
            }
        ]
      ],
    }
  ]
}
