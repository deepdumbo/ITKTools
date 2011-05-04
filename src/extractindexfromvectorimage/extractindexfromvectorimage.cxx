/*=========================================================================
*
* Copyright Marius Staring, Stefan Klein, David Doria. 2011.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0.txt
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*=========================================================================*/
/** \file
 \brief This program extracts a user specified component from a vector image.
 
 \verbinclude extractindexfromvectorimage.help
 */
#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImageFileReader.h"
#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkImageFileWriter.h"

#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkVector.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run( function, type, dim ) \
if ( ComponentTypeIn == #type && Dimension == dim ) \
{ \
  function< type, dim >( inputFileName, outputFileName, index ); \
  supported = true; \
}

//-------------------------------------------------------------------------------------

/** Declare ExtractIndex. */
template< class TPixel, unsigned int NDimension >
void ExtractIndex(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int & index );

/** Declare GetHelpString. */
std::string GetHelpString( void );

//-------------------------------------------------------------------------------------

int main( int argc, char ** argv )
{
  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText(GetHelpString());

  parser->MarkArgumentAsRequired( "-in", "The input filename." );
  parser->MarkArgumentAsRequired( "-ind", "The index to extract." );

  itk::CommandLineArgumentParser::ReturnValue validateArguments = parser->CheckForRequiredArguments();

  if(validateArguments == itk::CommandLineArgumentParser::FAILED)
  {
    return EXIT_FAILURE;
  }
  else if(validateArguments == itk::CommandLineArgumentParser::HELPREQUESTED)
  {
    return EXIT_SUCCESS;
  }
  
  /** Get arguments. */
  std::string inputFileName = "";
  parser->GetCommandLineArgument( "-in", inputFileName );

  std::string outputFileName = inputFileName.substr( 0, inputFileName.rfind( "." ) );
  outputFileName += "INDEXEXTRACTED.mhd";
  parser->GetCommandLineArgument( "-out", outputFileName );

  unsigned int index = 0;
  parser->GetCommandLineArgument( "-ind", index );

  /** Determine image properties. */
  std::string ComponentTypeIn = "short";
  std::string PixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int NumberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = GetImageProperties(
    inputFileName,
    PixelType,
    ComponentTypeIn,
    Dimension,
    NumberOfComponents,
    imagesize );
  if ( retgip != 0 )
  {
    std::cerr << "ERROR: error while getting image properties of the input image!" << std::endl;
    return 1;
  }

  /** Check for vector images. */
  if ( NumberOfComponents == 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is 1!" << std::endl;
    std::cerr << "Cannot make extract index from a scalar image." << std::endl;
    return 1;
  }

  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( ComponentTypeIn );

  /** Sanity check. */
  if ( index > NumberOfComponents - 1 )
  {
    std::cerr << "ERROR: You selected index "
      << index << ", where the input image only has "
      << NumberOfComponents << " components." << std::endl;
    return 1;
  }

  /** Run the program. */
  bool supported = false;
  try
  {
    run( ExtractIndex, char, 2 );
    run( ExtractIndex, unsigned char, 2 );
    run( ExtractIndex, short, 2 );
    run( ExtractIndex, unsigned short, 2 );
    run( ExtractIndex, int, 2 );
    run( ExtractIndex, unsigned int, 2 );
    run( ExtractIndex, long, 2 );
    //run( ExtractIndex, unsigned long, 2 );
    run( ExtractIndex, float, 2 );
    run( ExtractIndex, double, 2 );

    run( ExtractIndex, char, 3 );
    run( ExtractIndex, unsigned char, 3 );
    run( ExtractIndex, short, 3 );
    run( ExtractIndex, unsigned short, 3 );
    run( ExtractIndex, int, 3 );
    run( ExtractIndex, unsigned int, 3 );
    run( ExtractIndex, long, 3 );
    //run( ExtractIndex, unsigned long, 3 );
    run( ExtractIndex, float, 3 );
    run( ExtractIndex, double, 3 );
  }
  catch( itk::ExceptionObject &e )
  {
    std::cerr << "Caught ITK exception: " << e << std::endl;
    return 1;
  }
  if ( !supported )
  {
    std::cerr << "ERROR: this combination of pixeltype and dimension is not supported!" << std::endl;
    std::cerr
      << "pixel (component) type = " << ComponentTypeIn
      << " ; dimension = " << Dimension
      << std::endl;
    return 1;
  }

  /** End program. */
  return 0;

} // end main


/**
 * ******************* ExtractIndex *******************
 */

template< class TPixel, unsigned int NDimension >
void ExtractIndex(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int & index )
{
  /** Typedef's. */
  typedef TPixel                 PixelType;
  const unsigned int Dimension = NDimension;

  //typedef itk::Vector< PixelType, Dimension >         VectorPixelType;
  //typedef itk::Image< VectorPixelType, Dimension >    VectorImageType;
  /** Use vector image type that dynamically determines vector length: */
  typedef itk::VectorImage< PixelType, Dimension >    VectorImageType;
  typedef itk::Image< PixelType, Dimension >          ImageType;
  typedef itk::ImageFileReader< VectorImageType >     ImageReaderType;
  typedef itk::VectorIndexSelectionCastImageFilter<
    VectorImageType, ImageType >                      IndexExtractorType;
  typedef itk::ImageFileWriter< ImageType >           ImageWriterType;

  /** Read input image. */
  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName( inputFileName );

  /** Extract index. */
  typename IndexExtractorType::Pointer extractor = IndexExtractorType::New();
  extractor->SetInput( reader->GetOutput() );
  extractor->SetIndex( index );

  /** Write output image. */
  typename ImageWriterType::Pointer writer = ImageWriterType::New();
  writer->SetFileName( outputFileName );
  writer->SetInput( extractor->GetOutput() );
  writer->Update();

} // end ExtractIndex()


/**
 * ******************* GetHelpString *******************
 */

std::string GetHelpString()
{
  std::stringstream ss;
  ss << "This program extracts a user specified component from a vector image." << std::endl
  << "Usage:" << std::endl
  << "pxextractindexfromvectorimage" << std::endl
  << "  -in      inputFilename" << std::endl
  << "  [-out]   outputFilename, default in + INDEXEXTRACTED.mhd" << std::endl
  << "  -ind     a valid index" << std::endl
  << "Supported: 2D, 3D, (unsigned) char, (unsigned) short, (unsigned) int," << std::endl
  << "long, float, double.";
  return ss.str();
} // end GetHelpString()

