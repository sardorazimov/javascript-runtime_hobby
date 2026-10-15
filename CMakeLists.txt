# CMakeLists.txt: Tüm bu C++ dosyalarını ve V8 kütüphanesini 
# birbirine bağlayıp çalıştırılabilir binary (exe) haline getiren derleme talimatıdır.
cmake_minimum_required(VERSION 3.15)
project(MiniNodeRuntime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(MININODE_V8_ROOT "" CACHE PATH "Path to a V8 SDK root")
set(MININODE_V8_LIBRARIES "" CACHE STRING "Semicolon-separated V8 libraries to link")

set(MININODE_V8_HINTS "${MININODE_V8_ROOT}")
if(APPLE)
	list(APPEND MININODE_V8_HINTS
		/opt/homebrew/opt/v8
		/usr/local/opt/v8
	)
endif()

find_path(MININODE_V8_INCLUDE_DIR
	NAMES v8.h
	HINTS ${MININODE_V8_HINTS}
	PATH_SUFFIXES include include/node libexec/include
)

if(NOT MININODE_V8_LIBRARIES)
	find_library(MININODE_V8_MONOLITH_LIBRARY
		NAMES v8_monolith
		HINTS ${MININODE_V8_HINTS}
		PATH_SUFFIXES lib lib64 libexec
	)
	find_library(MININODE_V8_LIBRARY
		NAMES v8
		HINTS ${MININODE_V8_HINTS}
		PATH_SUFFIXES lib lib64 libexec
	)
	find_library(MININODE_V8_PLATFORM_LIBRARY
		NAMES v8_libplatform
		HINTS ${MININODE_V8_HINTS}
		PATH_SUFFIXES lib lib64 libexec
	)
	find_library(MININODE_V8_BASE_LIBRARY
		NAMES v8_libbase
		HINTS ${MININODE_V8_HINTS}
		PATH_SUFFIXES lib lib64 libexec
	)

	if(MININODE_V8_MONOLITH_LIBRARY AND MININODE_V8_PLATFORM_LIBRARY)
		set(MININODE_V8_LIBRARIES
			"${MININODE_V8_PLATFORM_LIBRARY};${MININODE_V8_MONOLITH_LIBRARY}")
	elseif(MININODE_V8_LIBRARY AND MININODE_V8_PLATFORM_LIBRARY)
		set(MININODE_V8_LIBRARIES
			"${MININODE_V8_PLATFORM_LIBRARY};${MININODE_V8_LIBRARY}")
	endif()
	if(MININODE_V8_BASE_LIBRARY)
		list(APPEND MININODE_V8_LIBRARIES "${MININODE_V8_BASE_LIBRARY}")
	endif()
endif()

set(MININODE_SOURCES
	async_task.cpp
	buffer.cpp
	console_module.cpp
	event_loop.cpp
	fs_module.cpp
	main.cpp
	module_loader.cpp
	net_module.cpp
	runtime.cpp
)

if(MININODE_V8_INCLUDE_DIR AND MININODE_V8_LIBRARIES)
	add_executable(mininode ${MININODE_SOURCES})
	target_include_directories(mininode PRIVATE "${MININODE_V8_INCLUDE_DIR}")
	target_link_libraries(mininode PRIVATE ${MININODE_V8_LIBRARIES})
else()
	add_custom_target(mininode SOURCES ${MININODE_SOURCES})
	message(STATUS
		"V8 SDK libraries not found; configured mininode source target only. "
		"Set MININODE_V8_ROOT and MININODE_V8_LIBRARIES to build the executable.")
endif()