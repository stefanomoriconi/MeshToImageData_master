cmake_minimum_required(VERSION 2.8)

PROJECT(MeshToImageData)

find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
if (ITKVtkGlue_LOADED)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
else()
  find_package(ItkVtkGlue REQUIRED)
  include(${ItkVtkGlue_USE_FILE})
  set(Glue ItkVtkGlue)
endif()

add_executable(MeshToImageData MeshToImageData.cpp)

if(VTK_LIBRARIES)
  target_link_libraries(MeshToImageData ${VTK_LIBRARIES} ${Glue} ${ITK_LIBRARIES})
else()
  target_link_libraries(MeshToImageData vtkHybrid vtkWidgets)
endif()