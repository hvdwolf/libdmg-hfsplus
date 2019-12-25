INCLUDE(FindZLIB)

IF(NOT ZLIB_FOUND)
	message(FATAL_ERROR "zlib is required!")
ENDIF(NOT ZLIB_FOUND)

add_library(dmg abstractfile.c base64.c checksum.c dmglib.c io.c resources.c udif.c)

target_link_libraries(dmg z)

add_executable(dmg-bin dmg.c)
target_link_libraries(dmg-bin dmg)

set_target_properties(dmg-bin PROPERTIES OUTPUT_NAME "dmg")

install(TARGETS dmg-bin DESTINATION bin)
