# CMake generated Testfile for 
# Source directory: C:/Users/user/OneDrive/Desktop/OOP/backend
# Build directory: C:/Users/user/OneDrive/Desktop/OOP/backend/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[AllRepoTests]=] "C:/Users/user/OneDrive/Desktop/OOP/backend/build/Debug/oop_tests.exe")
  set_tests_properties([=[AllRepoTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;62;add_test;C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[AllRepoTests]=] "C:/Users/user/OneDrive/Desktop/OOP/backend/build/Release/oop_tests.exe")
  set_tests_properties([=[AllRepoTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;62;add_test;C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[AllRepoTests]=] "C:/Users/user/OneDrive/Desktop/OOP/backend/build/MinSizeRel/oop_tests.exe")
  set_tests_properties([=[AllRepoTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;62;add_test;C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[AllRepoTests]=] "C:/Users/user/OneDrive/Desktop/OOP/backend/build/RelWithDebInfo/oop_tests.exe")
  set_tests_properties([=[AllRepoTests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;62;add_test;C:/Users/user/OneDrive/Desktop/OOP/backend/CMakeLists.txt;0;")
else()
  add_test([=[AllRepoTests]=] NOT_AVAILABLE)
endif()
