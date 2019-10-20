#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"

#include "itkHistogramMatchingImageFilter.h"
//#include "itkCastImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkDemonsRegistrationFilter.h"
#include "itkDisplacementFieldTransform.h"

// Setup types
const unsigned int Dim = 3 ;
using ImageType = itk::Image< float, Dim >;
using VectorPixelType = itk::Vector< float, Dim >;
using DisplacementFieldType = itk::Image< VectorPixelType, Dim >;
int NumOfIterations;
int ObserverIteration = 0;

std::string inputFixedName, inputMovingName, outputName;

class RegistrationObserver : public itk::Command {
public:
    using Self = RegistrationObserver ;
    using Superclass = itk::Command ;
    using Pointer = itk::SmartPointer < Self > ;
    itkNewMacro ( Self ) ;

    
    void Execute (itk::Object *caller, const itk::EventObject &event){
        //Execute((const itk::Object *)caller, event);
        auto * demons = static_cast<DemonsRegistrationType *>(caller);
        
        if (!itk::IterationEvent().CheckEvent(&event)){
            return;
        }
        
        std::cout << "Current iteration: " << ObserverIteration << "\n";
        ++ObserverIteration;


        const ImageType * fixedImage = dynamic_cast< const ImageType * >(demons->GetFixedImage()) ;
        const ImageType * movingImage = dynamic_cast< const ImageType *>(demons->GetMovingImage());
        DisplacementFieldType * output = static_cast< DisplacementFieldType *>( demons->GetOutput() );
        
        
        using DisplacementFieldTransform = itk::DisplacementFieldTransform< float , Dim >;
        DisplacementFieldTransform::Pointer displacement = DisplacementFieldTransform::New();
        displacement->SetDisplacementField( demons->GetOutput() );

        // Setting up rescaler
        using ResampleFilterType = itk::ResampleImageFilter< ImageType, ImageType , float, float >;
        ResampleFilterType::Pointer resample = ResampleFilterType::New();
        resample->SetInput( movingImage );
        resample->UseReferenceImageOn();
        resample->SetReferenceImage( fixedImage );
        resample->SetSize( movingImage->GetLargestPossibleRegion().GetSize() );
        resample->SetTransform( displacement );
        resample->Update();

        
        std::string name = "iteration_";
        name.append( std::to_string(ObserverIteration) ).append("_");
        name.append( outputName );

        std::cout << "Writing out to: " << name << "\n";

        // write out the result to argv[3]
        using writerType = itk::ImageFileWriter < ImageType > ;
        writerType::Pointer writer = writerType::New() ;
        writer->SetFileName( name );
        writer->SetInput( resample->GetOutput() );
        writer->Update() ;
    }

    void Execute (const itk::Object *caller, const itk::EventObject &event){
        
    }
protected:
	RegistrationObserver() {};
    
    using DemonsRegistrationType = itk::DemonsRegistrationFilter<   ImageType, 
                                                                    ImageType, 
                                                                    DisplacementFieldType >;
};

int main(int argc, char * argv[])
{
    std::cout << "Starting deformed filter\n";
    std::cout << "Assume first argument is number of moving images\n";

    // Verify command line arguments
    if( argc < 5 ){
        std::cerr << "Usage: " << std::endl;
        std::cerr << argv[0] << " inputFixedImage inputMovingImageS outputRegisteredMovingImage NumOfIterations" << std::endl; 
        return EXIT_FAILURE ;
    }

    inputFixedName = argv[1];
    inputMovingName = argv[2];
    outputName = "deformed_";
    outputName.append( argv[3] );
    NumOfIterations = atoi( argv[4] );
    
    std::cout << "Reading in fixed: " << inputFixedName << "\n";
    std::cout << "Reading in moving: " << inputMovingName << "\n"; 

    // Create and setup reader
    using ReaderType = itk::ImageFileReader< ImageType >;

    ReaderType::Pointer fixedReader = ReaderType::New();
    ReaderType::Pointer movingReader = ReaderType::New();

    fixedReader->SetFileName( inputFixedName );
    movingReader->SetFileName( inputMovingName );
    
    fixedReader->Update() ;
    movingReader->Update();
    
    ImageType::Pointer fixedImage = fixedReader->GetOutput() ;
    ImageType::Pointer movingImage = movingReader->GetOutput();


    // use histogram matching filter to match the two images
    // demons require pixels for homologous points have the same intensity
    using MatchingFilterType = itk::HistogramMatchingImageFilter<ImageType, ImageType>;
    MatchingFilterType::Pointer matchFilter = MatchingFilterType::New();

    matchFilter->SetInput( movingImage );
    matchFilter->SetReferenceImage( fixedImage );

    matchFilter->SetNumberOfHistogramLevels( 1024 );
    matchFilter->SetNumberOfMatchPoints( 7 );

    //matchFilter->ThresholdAtMeanIntensityOn();
    matchFilter->Update();
    

    // Setting up demons registration filter
    using DemonsRegistrationType = itk::DemonsRegistrationFilter< ImageType, ImageType, DisplacementFieldType >;
    DemonsRegistrationType::Pointer demons = DemonsRegistrationType::New();
    demons->SetFixedImage( fixedImage );
    demons->SetMovingImage( matchFilter->GetOutput() );
    demons->SetNumberOfIterations( NumOfIterations );
    demons->SetStandardDeviations( 1 );

    // Set up observer
    RegistrationObserver::Pointer observer = RegistrationObserver::New();
    demons->AddObserver(itk::IterationEvent(), observer);
    
    // Run the registration
    try{
        demons->Update() ;
    } catch ( itk::ExceptionObject & excp ){
        std::cerr << "Error in registration" << std::endl;
        std::cerr << excp << std::endl;
    }

    using DisplacementFieldTransform = itk::DisplacementFieldTransform< float , Dim >;
    DisplacementFieldTransform::Pointer displacement = DisplacementFieldTransform::New();
    displacement->SetDisplacementField( demons->GetOutput() );

    // Setting up rescaler
    using ResampleFilterType = itk::ResampleImageFilter< ImageType, ImageType , float, float >;
    ResampleFilterType::Pointer resample = ResampleFilterType::New();
    resample->SetInput( movingImage );
    resample->UseReferenceImageOn();
    resample->SetReferenceImage( fixedImage );
    resample->SetSize( movingImage->GetLargestPossibleRegion().GetSize() );
    resample->SetTransform( displacement );
    resample->Update();

    std::cout << "Writing out to: " << outputName << "\n";

    // write out the result to argv[3]
    using writerType = itk::ImageFileWriter < ImageType > ;
    writerType::Pointer writer = writerType::New() ;
    writer->SetFileName( outputName );
    writer->SetInput( resample->GetOutput() );
    writer->Update() ;

    // Done.
    std::cout << "Program finished.\n";
    return 0 ;
}
