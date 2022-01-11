#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkSphereSource.h>
#include <vtkMetaImageWriter.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>

#include <vtkSTLReader.h>
#include <itkImage.h>
#include <itkVTKImageToImageFilter.h>
#include <itkImageFileWriter.h>

/**
 * This program generates raster image volume from a surface segmentation.
   It gets as input: 
   - a filename (string) of watertight closed surface, as vtkPolyData (.vtp or .stl) 
   - a numeric value (float) of the isotropic image voxelsize [mm]
   - a filename (string) of the output image volume (.mhd or .nii).
  The foreground voxels are 255 and the background voxels are set to 0. 
  Internally vtkPolyDataToImageStencil is utilized.
 */
int main(int argc, char *argv[])
{  
  if (argc < 4)
    {
      std::cout << "" << std::endl;
      std::cout << "Usage: " << argv[0] << " inputMesh(.vtp or .stl) isoVoxelSize[mm] outputImage(.mhd or .nii)" << std::endl;
      std::cout << "" << std::endl;
      return EXIT_FAILURE;
    }

  std::string inputMeshFilename = argv[1];
  double isoVoxelSize = atof(argv[2]);
  std::string outputImageFilename = argv[3];

  vtkSmartPointer<vtkPolyData> pd = vtkSmartPointer<vtkPolyData>::New();

  if (strcmp(strrchr(argv[1],'.'),".stl") == 0){
      vtkSmartPointer<vtkSTLReader> inputMeshReader = vtkSmartPointer<vtkSTLReader>::New();
      inputMeshReader->SetFileName( inputMeshFilename.c_str() );
      pd = inputMeshReader->GetOutput();
      inputMeshReader->Update();
  }
  else{
    if (strcmp(strrchr(argv[1],'.'),".vtp") == 0){
      // Loading the Mesh in .vtp format!!!
      vtkSmartPointer<vtkXMLPolyDataReader> inputMeshReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
      inputMeshReader->SetFileName( inputMeshFilename.c_str() );
      pd = inputMeshReader->GetOutput();
      inputMeshReader->Update();
    }
    else
    {
      std::cout << "" << std::endl;
      std::cout << "Input: " << argv[0] << " MUST be a Mesh (either .vtp or .stl)!" << std::endl;
      std::cout << "" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // New VTK image
  vtkSmartPointer<vtkImageData> whiteImage = 
    vtkSmartPointer<vtkImageData>::New();    
  double bounds[6];
  pd->GetBounds(bounds);
  bounds[0] -= fabs(bounds[0]*0.1); // Extra 10% Blank Margin
  bounds[1] += fabs(bounds[1]*0.1); // Extra 10% Blank Margin
  bounds[2] -= fabs(bounds[2]*0.1); // Extra 10% Blank Margin
  bounds[3] += fabs(bounds[3]*0.1); // Extra 10% Blank Margin
  bounds[4] -= fabs(bounds[4]*0.1); // Extra 10% Blank Margin
  bounds[5] += fabs(bounds[5]*0.1); // Extra 10% Blank Margin
  double spacing[3]; // desired volume spacing
  spacing[0] = isoVoxelSize;
  spacing[1] = isoVoxelSize;
  spacing[2] = isoVoxelSize;
  whiteImage->SetSpacing(spacing);

  // compute dimensions
  int dim[3];
  for (int i = 0; i < 3; i++)
    {
    dim[i] = static_cast<int>(ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i]));
    }
  whiteImage->SetDimensions(dim);
  whiteImage->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);

  double origin[3];
  origin[0] = bounds[0] + spacing[0] / 2;
  origin[1] = bounds[2] + spacing[1] / 2;
  origin[2] = bounds[4] + spacing[2] / 2;
  whiteImage->SetOrigin(origin);

#if VTK_MAJOR_VERSION <= 5
  whiteImage->SetScalarTypeToUnsignedChar();
  whiteImage->AllocateScalars();
#else
  whiteImage->AllocateScalars(VTK_UNSIGNED_CHAR,1);
#endif
  // fill the image with foreground voxels:
  unsigned char inval = 255;
  unsigned char outval = 0;
  vtkIdType count = whiteImage->GetNumberOfPoints();
  for (vtkIdType i = 0; i < count; ++i)
    {
    whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);
    }

  // polygonal data --> image stencil:
  vtkSmartPointer<vtkPolyDataToImageStencil> pol2stenc = 
    vtkSmartPointer<vtkPolyDataToImageStencil>::New();
#if VTK_MAJOR_VERSION <= 5
  pol2stenc->SetInput(pd);
#else
  pol2stenc->SetInputData(pd);
#endif
  pol2stenc->SetOutputOrigin(origin);
  pol2stenc->SetOutputSpacing(spacing);
  pol2stenc->SetOutputWholeExtent(whiteImage->GetExtent());
  pol2stenc->Update();

  // cut the corresponding white image and set the background:
  vtkSmartPointer<vtkImageStencil> imgstenc = 
    vtkSmartPointer<vtkImageStencil>::New();
#if VTK_MAJOR_VERSION <= 5
  imgstenc->SetInput(whiteImage);
  imgstenc->SetStencil(pol2stenc->GetOutput());
#else
  imgstenc->SetInputData(whiteImage);
  imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
#endif
  imgstenc->ReverseStencilOff();
  imgstenc->SetBackgroundValue(outval);
  imgstenc->Update();

if (strcmp(strrchr(argv[3],'.'),".nii") == 0){
  // Converting VTK image to ITK image
  typedef itk::Image< unsigned char, 3 > ImageType;
  typedef itk::VTKImageToImageFilter<ImageType> vtk2itkImageFilterType;
  vtk2itkImageFilterType::Pointer vtk2itkImageFilter = vtk2itkImageFilterType::New();
  
  vtk2itkImageFilter->SetInput(imgstenc->GetOutput());
  vtk2itkImageFilter->Update();
  // ITK Image WRITER
  typedef itk::ImageFileWriter< ImageType > WriterType;
  WriterType::Pointer NiftiWriter = WriterType::New();
  
  NiftiWriter->SetFileName(strcat(argv[3],".gz"));
  NiftiWriter->SetInput(vtk2itkImageFilter->GetOutput());
  NiftiWriter->Update();
}
else{
  if (strcmp(strrchr(argv[3],'.'),".mhd") == 0){
    vtkSmartPointer<vtkMetaImageWriter> writer = 
    vtkSmartPointer<vtkMetaImageWriter>::New();
    writer->SetFileName( outputImageFilename.c_str() );
    #if VTK_MAJOR_VERSION <= 5
    writer->SetInput(imgstenc->GetOutput());
    #else
    writer->SetInputData(imgstenc->GetOutput());
    #endif
    writer->Write();  
  }
  else{
    std::cout << "" << std::endl;
    std::cout << "Input: " << argv[3] << " MUST be an Image (either .mhd or .nii)!" << std::endl;
    std::cout << "" << std::endl;
    return EXIT_FAILURE;
    }
}
  
  return EXIT_SUCCESS;
}