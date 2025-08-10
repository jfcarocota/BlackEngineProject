# Copy runtime DLLs if provided
if(dlls)
  foreach(dll IN LISTS dlls)
    if(EXISTS "${dll}")
      file(COPY "${dll}" DESTINATION "${dest}")
    endif()
  endforeach()
endif()
