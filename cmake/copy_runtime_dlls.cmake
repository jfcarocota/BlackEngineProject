# Copy runtime DLLs if provided
if(dlls)
  foreach(dll IN LISTS dlls)
    if(EXISTS "${dll}")
      file(COPY "${dll}" DESTINATION "${dest}")
    endif()
  endforeach()
endif()

# Copy additional Visual C++ runtime DLLs if they exist
set(VCREDIST_DLLS 
    "C:/Windows/System32/msvcp140.dll"
    "C:/Windows/System32/vcruntime140.dll"
    "C:/Windows/System32/vcruntime140_1.dll"
)

foreach(VCREDIST_DLL ${VCREDIST_DLLS})
    if(EXISTS "${VCREDIST_DLL}")
        get_filename_component(DLL_NAME "${VCREDIST_DLL}" NAME)
        message(STATUS "Copying Visual C++ runtime DLL: ${DLL_NAME}")
        file(COPY "${VCREDIST_DLL}" DESTINATION "${dest}")
    endif()
endforeach()
