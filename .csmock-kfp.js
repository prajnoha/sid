{
    "scan": {
        "analyzer-version-clang": "19.1.0",
        "analyzer-version-cppcheck": "2.16.0",
        "analyzer-version-gcc": "14.2.1",
        "enabled-plugins": "clang, cppcheck, gcc",
        "exit-code": 0,
        "host": "fed.virt",
        "known-false-positives": ".csmock-kfp.js",
        "mock-config": "default",
        "project-name": "sid-0.0.6-1.fc41",
        "store-results-to": "/root/rpmbuild/SRPMS/sid-0.0.6-1.fc41.tar.xz",
        "time-created": "2024-11-29 12:27:37",
        "time-finished": "2024-11-29 12:30:02",
        "tool": "csmock",
        "tool-args": "'/usr/bin/csmock' '--known-false-positives' '.csmock-kfp.js' '--cppcheck-add-flag=--check-level=exhaustive' '-r' 'default' '--tools' 'clang,cppcheck,gcc' 'sid-0.0.6-1.fc41.src.rpm'",
        "tool-version": "csmock-3.8.0-1.fc41"
    },
    "defects": [
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 1,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 0,
                    "event": "scope_hint",
                    "message": "In function 'int main(int, char**)'",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 165,
                    "column": 8,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'a1'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  165 |   auto a1 = 6538;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |        ^~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 172,
                    "column": 16,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'a4'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  172 |   decltype(a2) a4 = 34895.034;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                ^~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 176,
                    "column": 9,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'sa'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  176 |   short sa[cxx11test::get_val()] = { 0 };",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |         ^~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 180,
                    "column": 23,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'il'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  180 |   cxx11test::testinit il = { 4323, 435234.23544 };",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                       ^~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 201,
                    "column": 8,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'a'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  201 |   auto a = sum(1);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |        ^",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 202,
                    "column": 8,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'b'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  202 |   auto b = sum(1, 2);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |        ^",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 203,
                    "column": 8,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'c'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  203 |   auto c = sum(1.0, 2.0, 3.0);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |        ^",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 208,
                    "column": 25,
                    "event": "warning[-Wvexing-parse]",
                    "message": "empty parentheses were disambiguated as a function declaration",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  208 |   cxx11test::delegate d2();",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                         ^~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 208,
                    "column": 25,
                    "event": "note",
                    "message": "remove parentheses to default-initialize a variable",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  208 |   cxx11test::delegate d2();",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                         ^~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                         --",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 208,
                    "column": 25,
                    "event": "note",
                    "message": "or replace parentheses with braces to value-initialize a variable",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 217,
                    "column": 9,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'c'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  217 |   char *c = nullptr;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |         ^",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 225,
                    "column": 15,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'utf8'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  225 |   char const *utf8 = u8\"UTF-8 string \\u2500\";",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |               ^~~~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 226,
                    "column": 19,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'utf16'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  226 |   char16_t const *utf16 = u\"UTF-8 string \\u2500\";",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                   ^~~~~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "cwe": 563,
            "language": "c/c++",
            "tool": "gcc",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 227,
                    "column": 19,
                    "event": "warning[-Wunused-variable]",
                    "message": "unused variable 'utf32'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  227 |   char32_t const *utf32 = U\"UTF-32 string \\u2500\";",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                   ^~~~~",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 201,
                    "column": 8,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'a' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 201,
                    "column": 8,
                    "event": "note",
                    "message": "Value stored to 'a' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 202,
                    "column": 8,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'b' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 202,
                    "column": 8,
                    "event": "note",
                    "message": "Value stored to 'b' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 203,
                    "column": 8,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'c' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 203,
                    "column": 8,
                    "event": "note",
                    "message": "Value stored to 'c' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 225,
                    "column": 15,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'utf8' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 225,
                    "column": 15,
                    "event": "note",
                    "message": "Value stored to 'utf8' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 226,
                    "column": 19,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'utf16' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 226,
                    "column": 19,
                    "event": "note",
                    "message": "Value stored to 'utf16' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 227,
                    "column": 19,
                    "event": "warning[deadcode.DeadStores]",
                    "message": "Value stored to 'utf32' during its initialization is never read",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/conftest.cpp",
                    "line": 227,
                    "column": 19,
                    "event": "note",
                    "message": "Value stored to 'utf32' during its initialization is never read",
                    "verbosity_level": 2
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "language": "c/c++",
            "tool": "gcc",
            "hash_v1": "1bc5520e6e997ebe02281f3e675a888efe02f691",
            "key_event_idx": 1,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 0,
                    "event": "scope_hint",
                    "message": "In function '_do_sid_ucmd_dev_stack_get'",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 3903,
                    "column": 39,
                    "event": "warning[-Wuse-after-free]",
                    "message": "pointer 'strv1_39' used after 'free'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3903 |                                 strv  = strv1;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                                 ~~~~~~^~~~~~~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 3902,
                    "column": 33,
                    "event": "note",
                    "message": "call to 'free' here",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3902 |                                 free(strv);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                                 ^~~~~~~~~~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3901|   \t\t\tif (strv1 != strv) {",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3902|   \t\t\t\tfree(strv);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3903|-> \t\t\t\tstrv  = strv1;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3904|   \t\t\t\tcount = count1;",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 3905|   \t\t\t}",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "COMPILER_WARNING",
            "language": "c/c++",
            "tool": "gcc",
            "hash_v1": "ff0a12a77b437e87894902eb408f554917c5e381",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 845,
                    "column": 55,
                    "event": "warning[-Warray-bounds=]",
                    "message": "array subscript 5 is outside array bounds of 'struct kv_vector_t[5]'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  845 |                 vvalue[VVALUE_IDX_DATA_ALIGNED + idx] = (kv_vector_t) {data, data_size};",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 0,
                    "event": "scope_hint",
                    "message": "In function '_init_common'",
                    "verbosity_level": 1
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/ubr.c",
                    "line": 7186,
                    "column": 22,
                    "event": "note",
                    "message": "at offset 80 into object 'vvalue' of size 80",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": " 7186 |         kv_vector_t  vvalue[VVALUE_SINGLE_CNT];",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "      |                      ^~~~~~",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  843|   \tif (VVALUE_FLAGS(vvalue) & SID_KV_FL_ALIGN) {",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  844|   \t\tassert(vvalue_size >= VVALUE_IDX_DATA_ALIGNED + idx);",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  845|-> \t\tvvalue[VVALUE_IDX_DATA_ALIGNED + idx] = (kv_vector_t) {data, data_size};",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  846|   \t} else {",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  847|   \t\tassert(vvalue_size >= VVALUE_IDX_DATA + idx);",
                    "verbosity_level": 1
                }
            ]
        },
        {
            "checker": "CLANG_WARNING",
            "language": "c/c++",
            "tool": "clang",
            "hash_v1": "dd1f9ed25489877d3960a2caa3ef5b351d6cdeb6",
            "key_event_idx": 0,
            "events": [
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 512,
                    "column": 20,
                    "event": "warning[core.NonNullParamChecker]",
                    "message": "Null pointer passed to 1st parameter expecting 'nonnull'",
                    "verbosity_level": 0
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 804,
                    "column": 6,
                    "event": "note",
                    "message": "Assuming 'args' is non-null",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 804,
                    "column": 2,
                    "event": "note",
                    "message": "Taking false branch",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 6,
                    "event": "note",
                    "message": "Assuming the condition is false",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 6,
                    "event": "note",
                    "message": "Left side of '||' is false",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 63,
                    "event": "note",
                    "message": "Assuming field 'key' is non-null",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/include/internal/util.h",
                    "line": 76,
                    "column": 34,
                    "event": "note",
                    "message": "expanded from macro 'UTIL_STR_EMPTY'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 63,
                    "event": "note",
                    "message": "Left side of '||' is false",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/include/internal/util.h",
                    "line": 76,
                    "column": 34,
                    "event": "note",
                    "message": "expanded from macro 'UTIL_STR_EMPTY'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 63,
                    "event": "note",
                    "message": "Assuming the condition is false",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/include/internal/util.h",
                    "line": 76,
                    "column": 42,
                    "event": "note",
                    "message": "expanded from macro 'UTIL_STR_EMPTY'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/include/internal/util.h",
                    "line": 74,
                    "column": 34,
                    "event": "note",
                    "message": "expanded from macro 'UTIL_STR_END'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 807,
                    "column": 2,
                    "event": "note",
                    "message": "Taking false branch",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 813,
                    "column": 2,
                    "event": "note",
                    "message": "Value assigned to 'c_archive_key'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 817,
                    "column": 74,
                    "event": "note",
                    "message": "Assuming 'c_archive_key' is equal to NULL",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 820,
                    "column": 11,
                    "event": "note",
                    "message": "Calling '_unset_value'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 781,
                    "column": 2,
                    "event": "note",
                    "message": "Control jumps to 'case SID_KVS_BACKEND_HASH:'  at line 782",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 783,
                    "column": 8,
                    "event": "note",
                    "message": "Value assigned to 'relay.archive_arg.has_archive', which participates in a condition later",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 783,
                    "column": 8,
                    "event": "note",
                    "message": "Value assigned to 'relay.archive_arg.kv_store_value', which participates in a condition later",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 784,
                    "column": 4,
                    "event": "note",
                    "message": " Execution continues on line 791",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 791,
                    "column": 6,
                    "event": "note",
                    "message": "Assuming 'r' is >= 0",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 791,
                    "column": 6,
                    "event": "note",
                    "message": "Left side of '||' is false",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 791,
                    "column": 15,
                    "event": "note",
                    "message": "Assuming field 'ret_code' is >= 0",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 791,
                    "column": 2,
                    "event": "note",
                    "message": "Taking false branch",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 794,
                    "column": 2,
                    "event": "note",
                    "message": "Returning without writing to 'kv_store->backend', which participates in a condition later",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 820,
                    "column": 11,
                    "event": "note",
                    "message": "Returning from '_unset_value'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 820,
                    "column": 2,
                    "event": "note",
                    "message": "Taking false branch",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 823,
                    "column": 6,
                    "event": "note",
                    "message": "Assuming field 'has_archive' is true",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 823,
                    "column": 6,
                    "event": "note",
                    "message": "Left side of '&&' is true",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 823,
                    "column": 39,
                    "event": "note",
                    "message": "Assuming field 'kv_store_value' is non-null",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 823,
                    "column": 2,
                    "event": "note",
                    "message": "Taking true branch",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 830,
                    "column": 23,
                    "event": "note",
                    "message": "Passing null pointer value via 2nd parameter 'key'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 829,
                    "column": 12,
                    "event": "note",
                    "message": "Calling '_set_value'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 508,
                    "column": 2,
                    "event": "note",
                    "message": "Control jumps to 'case SID_KVS_BACKEND_HASH:'  at line 509",
                    "verbosity_level": 2
                },
                {
                    "file_name": "/builddir/build/BUILD/sid-0.0.6-build/sid-0.0.6/src/resource/kvs.c",
                    "line": 512,
                    "column": 20,
                    "event": "note",
                    "message": "Null pointer passed to 1st parameter expecting 'nonnull'",
                    "verbosity_level": 2
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  510|   \t\t\tr = hash_update(kv_store->ht,",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  511|   \t\t\t                key,",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  512|-> \t\t\t                strlen(key) + 1,",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  513|   \t\t\t                (void **) kv_store_value,",
                    "verbosity_level": 1
                },
                {
                    "file_name": "",
                    "line": 0,
                    "event": "#",
                    "message": "  514|   \t\t\t                kv_store_value_size,",
                    "verbosity_level": 1
                }
            ]
        }
    ]
}
