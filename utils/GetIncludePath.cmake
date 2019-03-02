FUNCTION(GET_INCLUDE_PATH _project_root _include_path_result)
	SET(RESULT_INCLUDE_PATH)
	FILE(GLOB_RECURSE ALL_HEADERS "${_project_root}/*.h")
	FOREACH(HEADER_FILE ${ALL_HEADERS})
		GET_FILENAME_COMPONENT(HEADER_PATH ${HEADER_FILE} DIRECTORY)
		LIST(APPEND RESULT_INCLUDE_PATH ${HEADER_PATH})
	ENDFOREACH(HEADER_FILE)

	SET(${_include_path_result} "${RESULT_INCLUDE_PATH}" PARENT_SCOPE)
ENDFUNCTION(GET_INCLUDE_PATH _project_root _include_path_result)