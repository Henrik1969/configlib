#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "configlib::shared" for configuration ""
set_property(TARGET configlib::shared APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(configlib::shared PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libconfiglib.so.0.10.0"
  IMPORTED_SONAME_NOCONFIG "libconfiglib.so.0"
  )

list(APPEND _cmake_import_check_targets configlib::shared )
list(APPEND _cmake_import_check_files_for_configlib::shared "${_IMPORT_PREFIX}/lib/libconfiglib.so.0.10.0" )

# Import target "configlib::static" for configuration ""
set_property(TARGET configlib::static APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(configlib::static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libconfiglib.a"
  )

list(APPEND _cmake_import_check_targets configlib::static )
list(APPEND _cmake_import_check_files_for_configlib::static "${_IMPORT_PREFIX}/lib/libconfiglib.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
