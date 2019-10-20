#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"

#include "itkDemonsRegistrationFilter.h"

// Setup types
const unsigned int nDims = 3 ;
using ImageType = itk::Image< float, nDims >;

class OptimizationObserver : public itk::Command {
public:
	using Self = OptimizationObserver;
	using Superclass = itk::Command;
	using Pointer = itk::SmartPointer< Self >;
	itkNewMacro( Self );
	
	void Execute (const itk::Object *caller, const itk::EventObject &event){
		OptimizerPointerType optimizer = dynamic_cast< OptimizerPointerType>( caller );
		std::cout << optimizer->GetCurrentIteration() << " " << optimizer->GetValue() << std::endl;
	};

	void Execute (itk::Object *caller, const itk::EventObject &event){
		Execute((const itk::Object*) caller, event);
	};
	using OptimizerType = itk::RegularStepGradientDescentOptimizer;
	using OptimizerPointerType = const OptimizerType *;
protected:
	OptimizationObserver() {};
	
};


class RegistrationObserver : public itk::Command {
public:
  using Self = RegistrationObserver ;
  using Superclass = itk::Command ;
  using Pointer = itk::SmartPointer < Self > ;
  itkNewMacro ( Self ) ;

  using OptimizerType = itk::RegularStepGradientDescentOptimizer ; 
  using OptimizerPointerType = OptimizerType * ;
  using RegistrationWrapperType  = itk::MultiResolutionImageRegistrationMethod < ImageType, ImageType > ;  // fixed, moving
  using RegistrationPointerType = RegistrationWrapperType * ;

  void Execute (itk::Object *caller, const itk::EventObject &event)
  {
    RegistrationPointerType registrationWrapper = dynamic_cast < RegistrationPointerType > ( caller ) ;
    OptimizerPointerType optimizer = dynamic_cast < OptimizerPointerType > ( registrationWrapper->GetModifiableOptimizer() ) ;

    std::cout << "RegObserver" << optimizer->GetCurrentIteration() << std::endl ;

    int level = registrationWrapper->GetCurrentLevel() ;
    if ( level == 0 ) 
       optimizer->SetMaximumStepLength ( 0.0625 ) ; 
    else if ( level == 1 ) 
       optimizer->SetMaximumStepLength ( 0.125 / 2 ) ; 
    else
       optimizer->SetMaximumStepLength ( 0.25 / 2 ) ; 

  }

  void Execute (const itk::Object *caller, const itk::EventObject &event)
  {
  }
protected:
	RegistrationObserver() {};
	
};

int main(int argc, char * argv[])
{
    std::cout << "Starting affine filter\n";
    std::cout << "Assume first argument is number of moving images\n";
    
    int imagesCount = atoi(argv[1]);
        


    // Verify command line arguments
    if( argc != imagesCount + 3 ){
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << " inputFixedImage inputMovingImageS outputRegisteredMovingImage" << std::endl; 
        return EXIT_FAILURE ;
    }

    std::string movingInputNames[imagesCount];
    std::string movingOutputNames[imagesCount];

    for (int i = 0; i < imagesCount; ++i){
        movingInputNames[i] = argv[i+3];
        movingOutputNames[i] = "affine_";
        movingOutputNames[i].append( movingInputNames[i] );
        std::cout << "Read in moving filenames " << movingInputNames[i] << "\n";
        std::cout << "\tCorresponding outputnames " << movingOutputNames[i] << "\n";
    }

    

 

    // Create and setup a reader for moving image
    using ReaderType = itk::ImageFileReader< ImageType >;
    
    // array of readers
    ReaderType::Pointer readerArr[imagesCount];
    
    // do all the reading
    for (int i = 0; i < imagesCount; ++i){
        readerArr[i] = ReaderType::New();
        readerArr[i]->SetFileName( movingInputNames[i] );
        readerArr[i]->Update();
        std::cout << "set up reader for " << readerArr[i]->GetFileName() << "\n";
    }
    

    // Same for the fixed image
    ReaderType::Pointer fixedReader = ReaderType::New() ;
    fixedReader->SetFileName ( argv[2] ) ;
    fixedReader->Update() ;
    ImageType::Pointer fixedImage = fixedReader->GetOutput() ;

    // Register images
    // Set up typedefs
    using TransformType = itk::AffineTransform< double, 3 >;
    using RegistrationWrapperType = itk::MultiResolutionImageRegistrationMethod< ImageType, ImageType >; // fixed , moving
    using LossFunctionType = itk::MeanSquaresImageToImageMetric< ImageType, ImageType >; // fixed , moving
    using InterpolationType = itk::LinearInterpolateImageFunction< ImageType, double >; // moving , coord
    using OptimizerType = itk::RegularStepGradientDescentOptimizer;

    // Declare the variables
    RegistrationWrapperType::Pointer registrationWrapper = RegistrationWrapperType::New();  

    TransformType::Pointer transform = TransformType::New();
    LossFunctionType::Pointer metric = LossFunctionType::New();
    InterpolationType::Pointer interpolator = InterpolationType::New();
    OptimizerType::Pointer optimizer = OptimizerType::New();

    // optimization observer
    OptimizationObserver::Pointer observer = OptimizationObserver::New();
    optimizer->AddObserver( itk::IterationEvent(), observer );

    // registration observer
    RegistrationObserver::Pointer regOb = RegistrationObserver::New();
    registrationWrapper->AddObserver( itk::IterationEvent(), regOb );

    std::cout << optimizer->GetMinimumStepLength() << " " << optimizer->GetMaximumStepLength() << " " << optimizer->GetNumberOfIterations() << " " << optimizer->GetGradientMagnitudeTolerance() << std::endl;
  
    


    // Connect the pipeline
    registrationWrapper->SetOptimizer( optimizer );
    registrationWrapper->SetMetric( metric );
    registrationWrapper->SetTransform( transform );
    registrationWrapper->SetInterpolator( interpolator );
    

    ImageType::Pointer movingImage ;
    for (int i = 0; i < imagesCount; ++i){
        movingImage = readerArr[i]->GetOutput();
        std::cout << "about to register image " << i << "\n";
        std::cout << "register ouput to " << movingOutputNames[i] << "\n";
        
        transform->SetIdentity();
        registrationWrapper->SetFixedImage( fixedImage );
        registrationWrapper->SetMovingImage( movingImage );

        registrationWrapper->SetInitialTransformParameters( transform->GetParameters() );
        registrationWrapper->SetFixedImageRegion( fixedImage->GetLargestPossibleRegion() );
        registrationWrapper->SetNumberOfLevels(3);

        // Run the registration
        try {  
            registrationWrapper->Update() ;
        }
        catch( itk::ExceptionObject & excp ) {
            std::cerr << "Error in registration" << std::endl;  
            std::cerr << excp << std::endl; 
        }

        // Update transform
        transform->SetParameters( registrationWrapper->GetLastTransformParameters() );

        // Apply the transform
        typedef itk::ResampleImageFilter < ImageType, ImageType > ResampleFilterType ;
        ResampleFilterType::Pointer filter = ResampleFilterType::New() ;
        filter->SetInput ( movingImage ) ;

        filter->SetTransform ( transform );
        filter->SetSize ( movingImage->GetLargestPossibleRegion().GetSize() ) ;
        filter->SetReferenceImage ( fixedImage ) ;
        filter->UseReferenceImageOn() ;
        filter->Update() ;

        // write out the result to argv[3]
        typedef itk::ImageFileWriter < ImageType > writerType ;
        writerType::Pointer writer = writerType::New() ;
        writer->SetInput ( filter->GetOutput() ) ; 
        writer->SetFileName ( movingOutputNames[i] ) ;
        writer->Update() ;

    }

    // Done.
    return 0 ;
}
