INCLUDE(FindZLIB)

IF(NOT ZLIB_FOUND)
	message(FATAL_ERROR "zlib is required for dmg!")
ENDIF(NOT ZLIB_FOUND)

link_directories(${PROJECT_BINARY_DIR}/common)

add_library(dmg adc.c base64.c checksum.c dmgfile.c dmglib.c io.c resources.c udif.c)

target_link_libraries(dmg common z)

add_executable(dmg-bin dmg.c)
target_link_libraries (dmg-bin dmg)

set_target_properties(dmg-bin PROPERTIES OUTPUT_NAME "dmg")

install(TARGETS dmg-bin DESTINATION bin)
