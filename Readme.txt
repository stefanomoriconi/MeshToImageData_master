***************************************
 MeshToImageData - by Stefano Moriconi
***************************************

[UNIX]

Requirements: ITK, VTK, (ITKVTKglue) correctly compiled (and installed)
 + ITK: https://itk.org
 + VTK: https://vtk.org

Configure first using: CMake (https://cmake.org) in a new /build subfolder

Move to the subfolder: cd /build

From build/ compile using: make

From build/ run (e.g.): ./MeshToImageData ./testData/Genus3.vtp 0.01 ./testData/Genus3.nii

Help: ./MeshToImageData 

-------------------------------------------------------
Mesh Input format: .vtp (VTK), .stl (Stereolitography)
Image Output format: .mhd (VTK), .nii (ITK)

NB: output .nii images will be compressed as nii.gz
