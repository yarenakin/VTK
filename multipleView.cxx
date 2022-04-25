#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkConeSource.h>
#include <vtkCubeSource.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkSTLReader.h>
#include <vtkColor.h>
#include <vtkCommonColorModule.h>
#include <vector>
#include <vtkOBJReader.h>
#include <vtkClipPolyData.h>
#include <vtkCommand.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkWindowToImageFilter.h>
#include <vtkContourFilter.h>
#include <vtkSplineFilter.h>
#include <vtkspline.h>
#include <vtkStripper.h>
#include <vtkKochanekSpline.h>
#include <vtkCutter.h>
#include <vtkTubeFilter.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkMetaImageWriter.h>
#include <vtkImageStencil.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkPointData.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkXMLImageDataReader.h>


std::string inputFilename_target = "C:/Users/yaren.akin/Desktop/Neslihan Prep.stl";
vtkNew<vtkSTLReader> reader_target;

std::string inputFilename_initial = "C:/Users/yaren.akin/Desktop/Neslihan Target.stl";
vtkNew<vtkSTLReader> reader_initial;


namespace {
    std::vector<vtkSmartPointer<vtkPolyDataAlgorithm>> GetSources();
}

namespace {
    class vtkIPWCallback : public vtkCommand
    {
    public:
        static vtkIPWCallback* New()
        {
            return new vtkIPWCallback;
        }
        virtual void Execute(vtkObject* caller, unsigned long, void*)
        {
            vtkImplicitPlaneWidget2* planeWidget =
                reinterpret_cast<vtkImplicitPlaneWidget2*>(caller);
            vtkImplicitPlaneRepresentation* rep =
                reinterpret_cast<vtkImplicitPlaneRepresentation*>(
                    planeWidget->GetRepresentation());
            rep->GetPlane(this->Plane);
        }
        vtkIPWCallback() : Plane(0), Actor(0)
        {
        }
        vtkPlane* Plane;
        vtkActor* Actor;
    };
} // namespace

int main(int, char* [])
{   
    /*STL dosyalarını okuma*/
    reader_target->SetFileName(inputFilename_target.c_str());
    reader_initial->SetFileName(inputFilename_initial.c_str());
    /************************/

    /* Portların gözükeceği koordinatları tanımlama*/
    double xmins[2] = { 0, .75};
    double xmaxs[2] = {1, 1};
    double ymins[2] = { 0, 0 };
    double ymaxs[2] = { 1, 0.45 };
    /***********************************************/

    /*1.port Kısmı  */
    vtkCamera* camera = nullptr;
    auto sources = GetSources();

    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> renderWindow;
    vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow(renderWindow);
    renderWindow->AddRenderer(renderer);

    renderer->SetViewport(xmins[0], ymins[0], xmaxs[0], ymaxs[0]);
    renderer->SetActiveCamera(camera);
    renderer->SetBackground(0.10, 0.40, 0.55);

    vtkNew<vtkXMLPolyDataReader> reader;

    /* Plane oluşturma ve açısını belirleme*/
    vtkNew<vtkPlane> plane;
    plane->SetOrigin(0, 0, 0);
    plane->SetNormal(1, 0, 0);
    /*                                     */

    /* İnitial ve target obje için 2 adet clipper oluşturma */
    vtkNew<vtkClipPolyData> clipper;
    vtkNew<vtkClipPolyData> clipper2;
    clipper->SetClipFunction(plane);
    clipper->InsideOutOff();
    clipper2->SetClipFunction(plane);
    clipper2->InsideOutOff();
    clipper->SetInputConnection(reader_initial->GetOutputPort());
    clipper2->SetInputConnection(reader_target->GetOutputPort());  

    //initial için line çizme
    vtkNew<vtkCutter> cutter;
    cutter->SetInputConnection(clipper->GetOutputPort());
    cutter->SetCutFunction(plane);
    cutter->GenerateValues(10, 0.5, 0.5);
    vtkNew<vtkCutter> cutter2;
    cutter2->SetInputConnection(clipper2->GetOutputPort());
    cutter2->SetCutFunction(plane);
    cutter2->GenerateValues(10, 0.5, 0.5);

    vtkNew<vtkStripper> stripper;
    stripper->SetInputConnection(cutter->GetOutputPort());
    stripper->JoinContiguousSegmentsOn();
    vtkNew<vtkStripper> stripper2;
    stripper2->SetInputConnection(cutter2->GetOutputPort());
    stripper2->JoinContiguousSegmentsOn();

    vtkNew<vtkPolyDataMapper> linesMapper;
    linesMapper->SetInputConnection(stripper->GetOutputPort());
    vtkNew<vtkPolyDataMapper> linesMapper2;
    linesMapper2->SetInputConnection(stripper2->GetOutputPort());

    vtkNew<vtkActor> lines;
    lines->SetMapper(linesMapper);
    lines->GetProperty()->SetDiffuseColor(1,0,0);
    lines->GetProperty()->SetLineWidth(3.0);
    vtkNew<vtkActor> lines2;
    lines2->SetMapper(linesMapper2);
    lines2->GetProperty()->SetDiffuseColor(0, 1, 1);
    lines2->GetProperty()->SetLineWidth(2.0);
    
    /*                                                         */
    /* initial ve target için mapper ve actor oluşturma */
    vtkNew<vtkPolyDataMapper> mapper_initial;
    vtkNew<vtkPolyDataMapper> mapper_target;
    mapper_initial->SetInputConnection(clipper->GetOutputPort());
    mapper_target->SetInputConnection(clipper2->GetOutputPort());
    vtkNew<vtkActor> actor_initial;
    vtkNew<vtkActor> actor_target;
    actor_initial->SetMapper(mapper_initial);
    actor_target->SetMapper(mapper_target);
    actor_target->GetProperty()->SetColor(1, 0, 1);
    /*                                                    */

    /* Objelere backface rengi verme ve render etme kısmı*/
    vtkNew<vtkProperty> backFaces;
    vtkNew<vtkProperty> backFaces2;
    backFaces->SetColor(0.30,0.14, 0.19);
    backFaces2->SetColor(0.25,1,0.45);

    actor_initial->SetBackfaceProperty(backFaces);
    actor_target->SetBackfaceProperty(backFaces2);

    renderer->AddActor(actor_initial);
    renderer->AddActor(actor_target);
    renderer->ResetCamera();
    /*                                                     */


    //2.port
    vtkNew<vtkRenderer> renderer2;

    renderWindow->AddRenderer(renderer2);
    renderer2->SetViewport(xmins[1], ymins[1], xmaxs[1], ymaxs[1]);
    renderer2->SetActiveCamera(camera);

    renderer2->AddActor(lines); //initial
    renderer2->AddActor(lines2);
    renderer2->ResetCamera();
    renderer2->SetBackground(0.10,0.40,0.55);
    //
  
    /* Burası kesme işlemi kısmı */
    // An interactor
    renderWindowInteractor->SetRenderWindow(renderWindow);

    renderWindow->Render();

    renderer->GetActiveCamera()->Azimuth(-180);
    renderer->GetActiveCamera()->Elevation(30);
    renderer->ResetCamera();
    renderer->GetActiveCamera()->Zoom(0.75);

    renderer2->GetActiveCamera()->Azimuth(-180);
    renderer2->GetActiveCamera()->Elevation(30);
    renderer2->ResetCamera();
    renderer2->GetActiveCamera()->Zoom(0.75);

    // The callback will do the work
    vtkNew<vtkIPWCallback> myCallback;
    myCallback->Plane = plane;
    myCallback->Actor = actor_initial;
    myCallback->Actor = actor_target;

    vtkNew<vtkImplicitPlaneRepresentation> rep;
    rep->SetPlaceFactor(1.25); 
    rep->PlaceWidget(actor_initial->GetBounds());
    rep->PlaceWidget(actor_target->GetBounds());
    rep->SetNormal(plane->GetNormal());
    rep->SetDrawOutline(false);

    vtkNew<vtkImplicitPlaneWidget2> planeWidget;
    planeWidget->SetInteractor(renderWindowInteractor);
    planeWidget->SetRepresentation(rep);
    planeWidget->AddObserver(vtkCommand::InteractionEvent, myCallback);


    // Render
    // 2 renderers and a render window
    renderWindowInteractor->Initialize();
    renderWindow->Render();
    planeWidget->On();

    // Begin mouse interaction
    renderWindowInteractor->Start();

    //svdsv
    // Extract the lines from the polydata
    vtkIdType numberOfLines = cutter->GetOutput()->GetNumberOfLines();

    std::cout << "-----------Lines without using vtkStripper" << std::endl;
    std::cout << "There are " << numberOfLines << " lines in the polydata"
        << std::endl;

    numberOfLines = stripper->GetOutput()->GetNumberOfLines();
    vtkPoints* points = stripper->GetOutput()->GetPoints();
    vtkCellArray* cells = stripper->GetOutput()->GetLines();

    std::cout << "-----------Lines using vtkStripper" << std::endl;
    std::cout << "There are " << numberOfLines << " lines in the polydata"
        << std::endl;


    return EXIT_SUCCESS;
}

namespace {
    std::vector<vtkSmartPointer<vtkPolyDataAlgorithm>> GetSources()
    {
        std::vector<vtkSmartPointer<vtkPolyDataAlgorithm>> sources;
        for (unsigned i = 0; i < 2; i++)
        {
            // target
            reader_target->Update();
            sources.push_back(reader_target);
            // initial
            reader_initial->Update();
            sources.push_back(reader_initial);
        }
        return sources;
    }
} // namespace

