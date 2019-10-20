#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkNaryAddImageFilter.h"
#include "itkShiftScaleImageFilter.h"


constexpr unsigned int Dim = 3;

int main(int argc, char * argv []){
    std::cout << "Starting average filter\n";
    std::cout << "Assume first argument is number of images\n";

    int imagesCount = atoi(argv[1]);
    
    //Setting up the image reader of the particular type
    using PixelType = float;
    using ImageType = itk::Image< PixelType, Dim >;
    using ReaderType = itk::ImageFileReader< ImageType >;

    // array of file names
    std::string filenameArr[imagesCount];
    std::string OutputFilename;
    // Reading in filenames
    if (argc == imagesCount + 3){
        for (int i = 0; i < imagesCount; ++i){
            filenameArr[i] = argv[i+2];
            std::cout << "Read in file " << filenameArr[i] << "\n";
        }        
        OutputFilename = argv[argc-1];
        std::cout << "Output is " << OutputFilename << "\n";
    } else {
        std::cout << "NumberOfImages inputImages outputName\n";
        std::cout << "or incorrect number of arguments\n";
        return EXIT_FAILURE;
    }

    // array of readers
    ReaderType::Pointer readerArr[imagesCount];

    // do all the reading
    for (int i = 0; i < imagesCount; ++i){
        readerArr[i] = ReaderType::New();
        readerArr[i]->SetFileName( filenameArr[i] );
        readerArr[i]->Update();
        std::cout << "set up reader for " << readerArr[i]->GetFileName() << "\n";
    }        

    
    // set up NaryAddImageFilter
    using NaryAddFilterType = itk::NaryAddImageFilter< ImageType, ImageType >;
    NaryAddFilterType::Pointer sum = NaryAddFilterType::New();

    for (int i = 0; i < imagesCount; ++i){
        sum->SetInput( i , readerArr[i]->GetOutput() );
        std::cout << "Add to sum image num " << i << "\n";
    }        
    
    sum->Update();

    double scale = (float) 1 / imagesCount;
    std::cout << "Multiply by: " << scale << std::endl;

    // set up scale filter
    using ScaleFilterType = itk::ShiftScaleImageFilter< ImageType, ImageType >;
    ScaleFilterType::Pointer scaleFilter = ScaleFilterType::New();
    scaleFilter->SetInput( sum->GetOutput() );
    scaleFilter->SetScale( scale );
    scaleFilter->Update();


    // setting up writer
    using WriterType = itk::ImageFileWriter < ImageType > ;
    WriterType::Pointer writer = WriterType::New() ;
    writer->SetFileName ( OutputFilename ) ;
    writer->SetInput ( scaleFilter->GetOutput() ) ;

    //Write to file
    try {
        writer->Update();
    } catch ( itk::ExceptionObject & error ){
        std::cerr << "Error: " << error << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Program finished succesfully\n";

    return 0;
}
