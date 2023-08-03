void Member::Nesting(vtkSmartPointer<vtkCylinderSource> disc)
{
    vtkNew<vtkNamedColors> colors;
    auto inputPolyData = m_filter->GetPolyDataOutput();
    auto polyDisc = disc->GetOutput();

    vtkNew<vtkCutter> circleCutter;
    circleCutter->SetInputData(inputPolyData);

    vtkNew<vtkPlane> cutPlane;
    cutPlane->SetOrigin(inputPolyData->GetCenter());
    cutPlane->SetNormal(0, 1, 0);
    circleCutter->SetCutFunction(cutPlane);
    circleCutter->Update();

    vtkNew<vtkStripper> stripper;
    stripper->SetInputConnection(circleCutter->GetOutputPort()); // valid circle
    stripper->SetJoinContiguousSegments(true);
    stripper->Update();

    auto circle = stripper->GetOutput();
    //ViewerWidget::GetViewer()->DisplayPolyData(circle,"Red");

    ///
    vtkNew<vtkConnectivityFilter> connectivityFilter;
    connectivityFilter->SetInputConnection(stripper->GetOutputPort());
    connectivityFilter->SetExtractionModeToLargestRegion(); // en büyük kontürü bul.
    connectivityFilter->Update();

    vtkSmartPointer<vtkGeometryFilter> largestContour = vtkSmartPointer<vtkGeometryFilter>::New();
    largestContour->SetInputConnection(connectivityFilter->GetOutputPort()); // poly dataya dönüştür.
    largestContour->Update();

    //ViewerWidget::GetViewer()->DisplayPolyData(largestContour->GetOutput(),"Red");
    ///

    double center[3];
    circle->GetCenter(center);
    double scaleFactor = 2.0;
    vtkNew<vtkTransform> transform;
    transform->Translate(center[0], center[1], center[2]);
    transform->Scale(scaleFactor, scaleFactor, scaleFactor);
    transform->Translate(-center[0], -center[1], -center[2]);

    // Transform the polydata
    vtkNew<vtkTransformPolyDataFilter> transformPD;
    transformPD->SetTransform(transform);
    transformPD->SetInputData(largestContour->GetOutput());
    transformPD->Update();

    auto offsetedContour = transformPD->GetOutput();
    ViewerWidget::GetViewer()->DisplayPolyData(offsetedContour,"Green");

    vtkNew<vtkLinearExtrusionFilter> extrudeFilterPositive;
    extrudeFilterPositive->SetInputData(offsetedContour);
    extrudeFilterPositive->SetExtrusionTypeToVectorExtrusion(); // Sadece z ekseninde derinlik eklemek için
    extrudeFilterPositive->SetVector(0.0, 10.0, 0.0); // Z yönünde ne kadar derinlik eklemek istediğinizi ayarlayın
    extrudeFilterPositive->Update();

    vtkNew<vtkLinearExtrusionFilter> extrudeFilterNegative;
    extrudeFilterNegative->SetInputData(offsetedContour);
    extrudeFilterNegative->SetExtrusionTypeToVectorExtrusion(); // Sadece z ekseninde derinlik eklemek için
    extrudeFilterNegative->SetVector(0.0, -10.0, 0.0); // -Z yönünde ne kadar derinlik eklemek istediğinizi ayarlayın
    extrudeFilterNegative->Update();

    vtkNew<vtkAppendPolyData> appendFilter;
    appendFilter->AddInputData(extrudeFilterPositive->GetOutput());
    appendFilter->AddInputData(extrudeFilterNegative->GetOutput());
    appendFilter->Update();

    //ViewerWidget::GetViewer()->DisplayPolyData(appendFilter->GetOutput(),"Blue");

    //burada image çevirme kısmı
    vtkNew<vtkAppendPolyData>append2Data;
    append2Data->AddInputData(inputPolyData);
    append2Data->AddInputData(polyDisc);
    append2Data->Update();

    vtkNew<vtkPolyDataConnectivityFilter>connectivity;
    connectivity->SetInputData( append2Data->GetOutput() );
    connectivity->Update();

    //ViewerWidget::GetViewer()->DisplayPolyData(connectivity->GetOutput(),"Blue");


    vtkSmartPointer<vtkImageData> linearExImage = ConvertPolydataToImage( appendFilter->GetOutput() );
    vtkSmartPointer<vtkImageData> discImage = ConvertPolydataToImage( polyDisc );

    std::vector<vtkSPtr<vtkImageData>> images;
    images.push_back( linearExImage );
    images.push_back( discImage );
    vtkSPtr<vtkImageData> appendImage = GetFinalImage( images, spacingVal );
    for( auto image : images )
    {
        AddImageData( appendImage, image );
    }
    // ================= finished: add image ==================

    // ============== convert image ===============

    SubImageData(appendImage,linearExImage);

    vtkSPtrNew( marchingCubes, vtkImageMarchingCubes );
    marchingCubes->SetInputData( appendImage );
    marchingCubes->SetValue( 0, (inval + outval)/2 );
    marchingCubes->SetNumberOfContours( 1 );
    marchingCubes->Update();

    // Fill the holes
    auto fillHoles = vtkSmartPointer<vtkFillHolesFilter>::New();
    fillHoles->SetInputConnection( marchingCubes->GetOutputPort() );
    fillHoles->SetHoleSize(1000.0);

    // Make the triangle winding order consistent
    auto normals = vtkSmartPointer<vtkPolyDataNormals>::New();
    normals->SetInputConnection(fillHoles->GetOutputPort());
    normals->ConsistencyOn();
    normals->SplittingOff();
    normals->GetOutput()->GetPointData()->SetNormals(
                marchingCubes->GetOutput()->GetPointData()->GetNormals());
    normals->Update();
    // ==============finished: convert image ===============

    vtkSPtrNew( connectivity2, vtkPolyDataConnectivityFilter );
    connectivity2->SetInputData( normals->GetOutput() );
    connectivity2->Update();

    ViewerWidget::GetViewer()->DisplayPolyData(connectivity2->GetOutput(),"Green");


}

void Member::SubImageData( vtkSmartPointer<vtkImageData> finalImage, vtkSmartPointer<vtkImageData> image )
{
    int *dim = image->GetDimensions();
    for( int i = 0; i < dim[0]; ++i )
    {
        for( int j = 0; j < dim[1]; ++j )
        {
            for( int k = 0; k < dim[2]; ++k )
            {
                char *pixel = static_cast<char *>(image->GetScalarPointer(i,j,k));
                if( pixel[0] == outval )
                {
                    continue;
                }
                int index[3] = {i, j, k};
                int pointId = image->ComputePointId( index );
                double pos[3];
                image->GetPoint( pointId, pos );
                int finalImagePointId = finalImage->FindPoint( pos );
                if( -1 != finalImagePointId )
                {
                    finalImage->GetPointData()->GetScalars()->SetTuple1( finalImagePointId, outval );
                }
            }
        }
    }
}
vtkSmartPointer<vtkImageData>Member::ConvertPolydataToImage(vtkPolyData *polydata)
{



    double bounds[6];
    polydata->GetBounds(bounds);

    int dim[3];
    for (int i = 0; i < 3; i++)
    {
        dim[i] = static_cast<int>( ceil((bounds[2 * i + 1] - bounds[2 * i]) / spacingVal) );
    }
    double origin[3];
    origin[0] = bounds[0] + spacingVal / 2;
    origin[1] = bounds[2] + spacingVal / 2;
    origin[2] = bounds[4] + spacingVal / 2;

    vtkSPtrNew(imageData, vtkImageData)
            imageData->SetSpacing(spacingVal, spacingVal, spacingVal);
    imageData->SetDimensions(dim);
    imageData->SetOrigin(origin);
    imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    imageData->SetExtent( 0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1 );
    // fill the imageData with foreground voxels
    vtkIdType count = imageData->GetNumberOfPoints();
    for (vtkIdType i = 0; i < count; ++i)
    {
        imageData->GetPointData()->GetScalars()->SetTuple1(i, inval);
    }

    // polygonal data --> imageData stencil:
    vtkSPtrNew(pd2stenc, vtkPolyDataToImageStencil);
    pd2stenc->SetInputData(polydata);
    pd2stenc->SetOutputOrigin(origin);
    pd2stenc->SetOutputSpacing(spacingVal, spacingVal, spacingVal);
    pd2stenc->SetOutputWholeExtent(imageData->GetExtent());
    pd2stenc->Update();

    // cut the corresponding white imageData and set the background:
    vtkSPtrNew(imgstenc, vtkImageStencil);
    imgstenc->SetInputData(imageData);
    imgstenc->SetStencilConnection(pd2stenc->GetOutputPort());
    imgstenc->ReverseStencilOff();
    imgstenc->SetBackgroundValue( outval );
    imgstenc->Update();

    imageData->DeepCopy(imgstenc->GetOutput());
    return imageData;

}
void Member::AddImageData( vtkSmartPointer<vtkImageData> finalImage, vtkSmartPointer<vtkImageData> image )
{
    int *dim = image->GetDimensions();
    for( int i = 0; i < dim[0]; ++i )
    {
        for( int j = 0; j < dim[1]; ++j )
        {
            for( int k = 0; k < dim[2]; ++k )
            {
                char *pixel = static_cast<char *>(image->GetScalarPointer(i,j,k));
                if( pixel[0] == outval )
                {
                    continue;
                }
                int index[3] = {i, j, k};
                int pointId = image->ComputePointId( index );
                double pos[3];
                image->GetPoint( pointId, pos );
                int finalImagePointId = finalImage->FindPoint( pos );
                if( -1 != finalImagePointId )
                {
                    finalImage->GetPointData()->GetScalars()->SetTuple1( finalImagePointId, inval );
                }
            }
        }
    }
}

vtkSmartPointer<vtkImageData> Member::GetFinalImage(std::vector<vtkSmartPointer<vtkImageData>> images, double spacingVal)
{
    double bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                         VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                         VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
    for( int i = 0; i < images.size(); ++i )
    {
        double origin[3];
        int dim[3];
        images[i]->GetOrigin( origin );
        images[i]->GetDimensions( dim );
        for( int j = 0; j < 3; ++j )
        {
            if( bounds[2*j] > origin[j] )
            {
                bounds[2*j] = origin[j];
            }
        }
        for( int j = 0; j < 3; ++j )
        {
            double boundsValue = origin[j] + dim[j] * spacingVal;
            if( boundsValue > bounds[2*j + 1] )
            {
                bounds[2*j + 1] = boundsValue;
            }
        }
    }
    int dim[3];
    for( int i = 0; i < 3; ++i )
    {
        dim[i] = static_cast<int>( ceil((bounds[2*i + 1] - bounds[2*i]) / spacingVal) );
    }
    double origin[3];
    origin[0] = bounds[0] + spacingVal / 2;
    origin[1] = bounds[2] + spacingVal / 2;
    origin[2] = bounds[4] + spacingVal / 2;

    vtkSPtrNew( finalImage, vtkImageData );
    finalImage->SetSpacing( spacingVal, spacingVal, spacingVal );
    finalImage->SetDimensions( dim );
    finalImage->SetOrigin( origin );
    finalImage->AllocateScalars( VTK_UNSIGNED_CHAR, 1 );

    vtkIdType count = finalImage->GetNumberOfPoints();
    for( vtkIdType i = 0; i < count; ++i )
    {
        finalImage->GetPointData()->GetScalars()->SetTuple1( i, outval );
    }
    return finalImage;
}
