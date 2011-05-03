#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImageFileReader.h"
#include "itkFlipImageFilter.h"
#include "itkImageFileWriter.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run( function, type, dim ) \
if ( ComponentType == #type && Dimension == dim ) \
{ \
  function< type, dim >( inputFileName, outputFileName, direction ); \
  supported = true; \
}

//-------------------------------------------------------------------------------------

/** Declare ReflectImageFilter. */
template< class TOutputPixel, unsigned int NDimension >
void ReflectImageFilter(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int direction );

/** Declare PrintHelp. */
std::string PrintHelp( void );

//-------------------------------------------------------------------------------------

int main( int argc, char ** argv )
{
  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText(PrintHelp());

  parser->MarkArgumentAsRequired( "-in", "The input filename." );
  parser->MarkArgumentAsRequired( "-out", "The output filename." );
  parser->MarkArgumentAsRequired( "-d", "Direction." );

  bool validateArguments = parser->CheckForRequiredArguments();

  if(!validateArguments)
  {
    return EXIT_FAILURE;
  }

  /** Get arguments. */
  std::string inputFileName = "";
  parser->GetCommandLineArgument( "-in", inputFileName );

  std::string outputFileName = "";
  parser->GetCommandLineArgument( "-out", outputFileName );

  unsigned int direction;
  parser->GetCommandLineArgument( "-d", direction );

  std::string ComponentType = "";
  bool retpt = parser->GetCommandLineArgument( "-opct", ComponentType );

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

  /** The default output is equal to the input, but can be overridden by
   * specifying -pt in the command line.   */
  if ( !retpt ) ComponentType = ComponentTypeIn;

  /** Check for vector images. */
  if ( NumberOfComponents > 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Cannot make vector of vector images." << std::endl;
    return 1;
  }

  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( ComponentType );

  /** Check direction. */
  if ( direction > Dimension - 1 )
  {
    std::cerr << "ERROR: invalid direction." << std::endl;
    return 1;
  }

  /** Run the program. */
  bool supported = false;
  try
  {
    run( ReflectImageFilter, char, 2 );
    run( ReflectImageFilter, unsigned char, 2 );
    run( ReflectImageFilter, short, 2 );
    run( ReflectImageFilter, unsigned short, 2 );
    run( ReflectImageFilter, int, 2 );
    run( ReflectImageFilter, unsigned int, 2 );
    run( ReflectImageFilter, long, 2 );
    run( ReflectImageFilter, unsigned long, 2 );
    run( ReflectImageFilter, float, 2 );
    run( ReflectImageFilter, double, 2 );

    run( ReflectImageFilter, char, 3 );
    run( ReflectImageFilter, unsigned char, 3 );
    run( ReflectImageFilter, short, 3 );
    run( ReflectImageFilter, unsigned short, 3 );
    run( ReflectImageFilter, int, 3 );
    run( ReflectImageFilter, unsigned int, 3 );
    run( ReflectImageFilter, long, 3 );
    run( ReflectImageFilter, unsigned long, 3 );
    run( ReflectImageFilter, float, 3 );
    run( ReflectImageFilter, double, 3 );

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
      << "pixel (component) type = " << ComponentType
      << " ; dimension = " << Dimension
      << std::endl;
    return 1;
  }

  /** End program. */
  return 0;

} // end main


/**
 * ******************* ReflectImageFilter *******************
 */

template< class TOutputPixel, unsigned int NDimension >
void ReflectImageFilter( const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int direction )
{
  /** Typedefs. */
  typedef TOutputPixel                                    OutputPixelType;
  const unsigned int Dimension = NDimension;

  typedef OutputPixelType                                 InputPixelType;

  typedef itk::Image< InputPixelType, Dimension >         InputImageType;
  typedef itk::Image< OutputPixelType, Dimension >        OutputImageType;

  typedef itk::ImageFileReader< InputImageType >          ReaderType;
  typedef itk::FlipImageFilter< InputImageType >          ReflectFilterType;
  typedef itk::ImageFileWriter< OutputImageType >         WriterType;

  /** Read in the input image. */
  typename ReaderType::Pointer reader = ReaderType::New();
  typename ReflectFilterType::Pointer reflectFilter = ReflectFilterType::New();
  typename WriterType::Pointer writer = WriterType::New();

  /** Set up pipeline. */
  reader->SetFileName( inputFileName );
  
  itk::FixedArray<bool, Dimension> flipAxes(false);
  flipAxes[direction] = true;
  
  reflectFilter->SetFlipAxes( flipAxes );
  writer->SetFileName( outputFileName );

  reflectFilter->SetInput( reader->GetOutput() );
  writer->SetInput( reflectFilter->GetOutput() );
  writer->Update();

} // end ReflectImageFilter()


/**
 * ******************* PrintHelp *******************
 */
std::string PrintHelp()
{
  std::string helpText = "This program reflects an image.\n \
  Usage:\n \
  pxreflect\n \
    -in      inputFilename\n \
    -out     outputFilename\n \
    -d       the image direction that should be reflected\n \
    [-opct]  output pixel type, default equal to input\n \
  Supported: 2D, 3D, (unsigned) char, (unsigned) short, (unsigned) int,\n \
  (unsigned) long, float, double.\n";

  return helpText;

} // end PrintHelp()

