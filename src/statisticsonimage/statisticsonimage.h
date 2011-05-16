#ifndef __statisticsonimage_h
#define __statisticsonimage_h

#include "itkImageFileReader.h"
#include "itkStatisticsImageFilterWithMask.h"
#include "itkCastImageFilter.h"
#include "itkGradientToMagnitudeImageFilter.h"
#include "itkScalarImageToHistogramGenerator2.h"
#include "itkMaskImageFilter.h"
#include "itkLogImageFilter.h"

#include "statisticsprinters.h"
#include "ITKToolsBase.h"
#include "ITKToolsHelpers.h"

/** This file defines three templated functions. */


/**
 * ******************* DetermineHistogramMaximum *******************
 *
 * Helper function.
 */

template< class PixelType >
void DetermineHistogramMaximum(
  const PixelType & maxPixelValue,
  const PixelType & minPixelValue,
  const unsigned int & numberOfBins,
  PixelType & histogramMax )
{
  typedef typename itk::NumericTraits<PixelType>::ScalarRealType RealPixelType;

  /** This code is copied from the ListSampleToHistogramGenerator->GenerateData()
   * and adapted. It makes sure that the maximum values are also included in the histogram.
   */

  /** Floating pixeltype. */
  if ( !itk::NumericTraits<PixelType>::is_integer )
  {
    /** If the maximum (almost) equals the minimum, we have to make sure that
     * everything still works.
     * 4 conditions:
     * - The binsize must be greater than epsilon
     * - The uppermargin must be greater than epsilon
     * - the histogramMax must be at least statistics->GetMaximum() + uppermargin
     * - the histogramMax must be at least numberOfBins * binsize
     * epsilon is chosen a little larger than the computer indicates,
     * to be on the safe side. The factor of 100.0 is determined empirically
     * to still give good results.
     */
    double marginalScale = 100.0;
    double epsilon = itk::NumericTraits<PixelType>::epsilon() * 100.0;

    double binsize = static_cast<double>( maxPixelValue - minPixelValue )
      / static_cast<double>( numberOfBins );
    binsize = vnl_math_max( binsize, epsilon );
    double uppermargin = vnl_math_max( epsilon, binsize / marginalScale );
    histogramMax = static_cast<RealPixelType>(
      vnl_math_max( binsize * static_cast<double>( numberOfBins ) + minPixelValue,
      maxPixelValue + uppermargin ) );
  }
  /** Integer pixeltypes. in principle this function will never be called
   * with an integer pixeltype, but just in case this is changed in the future ...
   */
  else
  {
    PixelType uppermargin = itk::NumericTraits<PixelType>::One;
    histogramMax = static_cast<RealPixelType>( maxPixelValue + uppermargin );
  }

  /** Check. */
  if ( histogramMax <= maxPixelValue )
  {
    /** Overflow occurred; maximum was already maximum of pixeltype;
     * We could solve this somehow (by adding a ClipBinsAtUpperBound(bool)
     * function to the itkScalarImageToHistogramGenerator2, and calling it
     * with argument 'false'), but the situation is quite unlikely; anyway,
     * mostly something is going wrong when a float image has value
     * infinity somewhere.
     */
    std::cerr << "Error during histogram computation!" << std::endl;
    std::cerr << "The maximum of the image is equal to the maximum of its pixeltype." << std::endl;
    std::cerr << "Histogram computation cannot be reliably performed now.";
    std::cerr << "  pxstatisticsonimage cannot handle this situation." << std::endl;
    itkGenericExceptionMacro( << "Histogram cannot be computed." );
  }

} // end DetermineHistogramMaximum()


/**
 * ************************ ComputeStatistics **************************8
 *
 * Generic template function that computes statistics on an input image
 * Assumes that the number of histogram bins and the marginal scale are set
 * in the histogramGenerator, that the maskerOrCopier has been initialized
 * to a (dummy) caster or to a MaskFilterType with mask set, and that the
 * statistics filter has been initialized.
 *
 * This function is only to be used by the StatisticsOnImage function.
 * It is quite specific and not really well over thought. Introduced to avoid
 * duplication of code.
 */

template< class TInputImage, class TBaseFilter, class TStatisticsFilter, class THistogramGenerator >
void ComputeStatistics(
  TInputImage * inputImage,
  TBaseFilter * maskerOrCopier,
  TStatisticsFilter * statistics,
  THistogramGenerator * histogramGenerator,
  unsigned int numberOfBins, // Type is defined in ScalarImageToHistogramGenerator2
  const std::string & histogramOutputFileName,
  const std::string & select )
{
  typedef THistogramGenerator                         HistogramGeneratorType;
  typedef typename
    HistogramGeneratorType::HistogramType             HistogramType;
  typedef typename HistogramGeneratorType::RealPixelType   RealPixelType;
  typedef TStatisticsFilter                           StatisticsFilterType;
  typedef typename StatisticsFilterType::PixelType    PixelType;
  typedef TInputImage                                 InputImageType;
  typedef itk::LogImageFilter<
    InputImageType, InputImageType>                   LogFilterType;

  /** Arithmetic mean */
  PixelType maxPixelValue = 1;
	PixelType minPixelValue = 0;
  if ( select == "arithmetic" || select == "" || select == "histogram" )
  {
    std::cout << "Computing arithmetic statistics ..." << std::endl;

    statistics->SetInput( inputImage );
    statistics->Update();

    /** Only print if not histogram selected. */
    if ( select != "histogram" )
    {
      PrintStatistics<StatisticsFilterType>( statistics );
    }
    if ( select == "arithmetic" ) return;

    /** Save for later use for the histogram bin size. */
    maxPixelValue = statistics->GetMaximum();
    minPixelValue = statistics->GetMinimum();
  }

  /** Geometric mean/std: */
  if ( select == "geometric" || select == "" )
  {
    std::cout << "Computing geometric statistics ..." << std::endl;

    typename LogFilterType::Pointer logger = LogFilterType::New();
    logger->SetInput( inputImage );
    statistics->SetInput( logger->GetOutput() );
    statistics->Update();

    PrintGeometricStatistics<StatisticsFilterType>( statistics );

    if ( select == "geometric" ) return;
  }

  /** Histogram statistics. */
  if ( select == "histogram" || select == "" )
  {
    /** Prepare for the histogram. */
    maskerOrCopier->SetInput( inputImage );
    std::string maskerOrCopierName = maskerOrCopier->GetNameOfClass();
    if ( maskerOrCopierName == "MaskImageFilter" )
    {
      std::cout << "Replacing all pixels outside the mask by -infinity,\n  ";
      std::cout << "to make sure they are not included in the histogram ..."
        << std::endl;
    }
    maskerOrCopier->Update();

    /** If the user specified 0, the number of bins is equal to the intensity range. */
    if ( numberOfBins == 0 )
    {
      numberOfBins = static_cast<unsigned int>( maxPixelValue - minPixelValue );
    }

    /** Determine histogram maximum. */
    PixelType histogramMax;
    DetermineHistogramMaximum( maxPixelValue, minPixelValue, numberOfBins, histogramMax );

    /** Computing histogram statistics. */
    std::cout << "Computing histogram statistics ..." << std::endl;

    histogramGenerator->SetAutoMinMax( false );
    histogramGenerator->SetNumberOfBins( numberOfBins );
    histogramGenerator->SetHistogramMin( static_cast<RealPixelType>( minPixelValue ) );
    histogramGenerator->SetHistogramMax( histogramMax );
    histogramGenerator->SetInput( maskerOrCopier->GetOutput() );
    histogramGenerator->Compute();

    PrintHistogramStatistics<HistogramType>(
      histogramGenerator->GetOutput(), histogramOutputFileName );
  }

} // end ComputeStatistics()

#endif // #ifndef __statisticsonimage_h

