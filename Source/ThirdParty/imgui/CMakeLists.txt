set (TARGET_NAME ImGui)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -Wall -Wextra -Wno-unused-parameter")
  add_definitions(-DMACOSX)
endif(APPLE)

file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})
set (INCLUDE_DIRS .)

setup_library ()
install_header_files (DIRECTORY ./ DESTINATION ${DEST_INCLUDE_DIR}/ThirdParty/ImGui FILES_MATCHING PATTERN *.h)  # Note: the trailing slash is significant
