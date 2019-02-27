set (TARGET_NAME TurboBadger)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -Wall -Wextra -Wno-unused-parameter")
  add_definitions(-DMACOSX)
endif(APPLE)

file (GLOB CPP_FILES *.cpp image/*.cpp animation/*.cpp parser/*.cpp renderers/*.cpp utf8/*.cpp tests/*.cpp)
file (GLOB H_FILES *.h image/*.h animation/*.h parser/*.h renderers/*.h utf8/*.h tests/*.h)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
set (INCLUDE_DIRS . image animation parser renderers utf8 tests)

setup_library ()
install_header_files (DIRECTORY . DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/TurboBadger FILES_MATCHING PATTERN *.h)  # Note: the trailing slash is significant
