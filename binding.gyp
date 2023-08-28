{
  "targets": [
    {
        "target_name": "zed",
        "sources": [],
        "defines": [],
        "cflags": ["-std=c++11", "-Wall", "-pedantic"],
        "include_dirs": [ 
          "<!(node -p \"require('node-addon-api').include_dir\")",
        ],
        "libraries": [],
        "dependencies": [],
        "conditions": [
            ['OS=="win"', {
              "sources": [ "node_zed.cpp" ],
              'include_dirs': [
                "$(CUDA_PATH)/include",
                "$(ZED_SDK_ROOT_DIR)/include"],
              'library_dirs': [
                '$(ZED_SDK_ROOT_DIR)/lib',
              ],
              'libraries': [
                'sl_zed64.lib'
              ],
              'msvs_settings': {
                'VCCLCompilerTool': { 'ExceptionHandling': 1 }
              }
            }],
            ['OS=="mac"', {
              'cflags+': ['-fvisibility=hidden'],
              'xcode_settings': {},
            }],
            ['OS=="linux"', {}],
        ],
    }
  ]
}