#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

//////////////////////////////////////////////////////////////////////////////////////////////////
cv::Mat computeHistogramGS(const cv::Mat &image){
    // create and initialize and 1d array of integers
    cv::Mat histogram(256, 1, CV_32S, cv::Scalar(0));

    // compute the histogram
    for(int i = 0; i < image.rows; i++){
        for(int j = 0; j < image.cols; j++){
            histogram.at<int>(image.at<unsigned char>(i, j), 0)++;
        }
    }
    return histogram;
}

cv::Mat histogramToImageGS(const cv::Mat &histogram){
    // create an image
    cv::Mat histogramImage(100, 256, CV_8U, cv::Scalar(0));

    // find the max value of the histogram
    double minValue, maxValue;
    cv::minMaxLoc(histogram, &minValue, &maxValue);
    maxValue *= 2;

    // write the histogram lines
    for(int j = 0; j < 256; j++){
        cv::line(histogramImage, cv::Point(j, 100), cv::Point(j, 100 - (100 * histogram.at<int>(j, 0)) / maxValue), cv::Scalar(255), 1);
    }
    return histogramImage;
}

cv::Mat computeCumulatedHistogramGS(const cv::Mat &image){
    // create and initialize and 1d array of integers
    cv::Mat cumul(256, 1, CV_32S,cv::Scalar(0));
    cv::Mat histogram = computeHistogramGS(image);

    // compute the histogram
    cumul.at<int>(0, 0) = histogram.at<int>(0, 0);
    for(int i = 1; i < histogram.rows; i++){
        cumul.at<int>(i, 0) =  histogram.at<int>(i, 0) + cumul.at<int>(i - 1, 0);
    }
    return cumul;
}

cv::Mat LUTimageEqualize(const cv::Mat &image){
    // create a LUT
    cv::Mat LUT(256, 1, CV_8U);
    cv::Mat histocumul = computeCumulatedHistogramGS(image);

    // find min and max value
    double minValue, maxValue;
    cv::minMaxLoc(image, &minValue, &maxValue);

    // compute the normalized image LUT
    for(int i = 0; i < 256; i++){
        LUT.at<unsigned char>(i) = (255.0 / (image.rows * image.cols)) * histocumul.at<int>(i, 0); 
    }
    return LUT;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
cv::Mat LUT(const cv::Mat &image, const cv::Mat &lut){
    // make a copy of image
    cv::Mat finalImage;
    image.copyTo(finalImage);

    // aplly the LUT
    for(int i = 0; i < image.rows; i++){
        for(int j = 0; j < image.cols; j++){
            finalImage.at<unsigned char>(i, j) = lut.at<unsigned char>(image.at<unsigned char>(i, j), 0); 
        }
    }
    return finalImage;
}
//////////////////////////////////////////////////////////////////////////////////////////////////

int compare(const void *a, const void *b) {
    return ((cv::Point3i *) a)->z - ((cv::Point3i *) b)->z;
}

cv::Mat imageEqualizeExact(const cv::Mat &image){
    //tableau des points (x->rows, y->cols, z->intensite)
    cv::Point3i *newImage = new cv::Point3i[image.cols * image.rows];

    // recupere les coordonnées et intensite de chaques pixels
    for (int i = 0; i < image.cols; i++) {
        for (int j = 0; j < image.rows; j++) {
            newImage[i * image.rows + j].x = i;
            newImage[i * image.rows + j].y = j;
            newImage[i * image.rows + j].z = image.at<unsigned char>(j, i);
        }
    }

    //trie le tableau de point
    qsort(newImage, image.cols * image.rows, sizeof(cv::Point3i), compare);

    //attribuer les intensités
    int pas = (image.cols * image.rows) / 256;
    int intensite = 0;
    for (int i = 0; i < (image.cols * image.rows); i++) {
        if (i % pas == 0 && i != 0) {
            intensite++;
        }
        newImage[i].z = intensite;
    }

    //refaire une image
    cv::Mat finalImage(image.rows, image.cols, CV_8U, cv::Scalar(0));
    
    int coordX, coordY, value;
    for (int i = 0; i < (image.rows * image.cols); i++) {
        coordX = newImage[i].x;
        coordY = newImage[i].y;
        value = newImage[i].z;
        finalImage.at<unsigned char>(coordY, coordX) = (unsigned char) value;
    }
    return finalImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char ** argv){
    // check arguments
    if(argc != 2){
        std::cout << "usage: " << argv[0] << " image" << std::endl;
        return -1;
    }

    // load the input image
    std::cout << "load image ..." << std::endl;
    cv::Mat image = cv::imread(argv[1]);
    if(image.empty()){
        std::cout << "error loading " << argv[1] << std::endl;
        return -1;
    }

    // verifie la taille de l'image faut sue ce soit un multiple de 256
    if ((image.cols * image.rows) % 256) {
        std::cout << "Error image: " << argv[1] << " because size is " << image.cols << " x " << image.rows << std::endl;
        return -1;
    }

    std::cout << "image size : " << image.cols << " x " << image.rows << std::endl;

    // convert the image to grayscale
    cvtColor(image, image, CV_BGR2GRAY);

    // display an image
    std::cout << "appuyer sur une touche ..." << std::endl;
    cv::imshow("Image Originale", image);
    cv::moveWindow ("Image Originale", 200, 0);
    cv::imshow("Histogramme Originale", histogramToImageGS(computeHistogramGS(image)));
    cv::moveWindow ("Histogramme Originale", 900, 100);
    cv::waitKey();

    // equalize the image
    cv::Mat equalizeImage = LUT(image,LUTimageEqualize(image));

    std::cout << "image size : " << equalizeImage.cols << " x " << equalizeImage.rows << std::endl;
    // display an image
    std::cout << "appuyer sur une touche ..." << std::endl;
    cv::imshow("Image Egalisation", equalizeImage);
    cv::moveWindow ("Image Egalisation", 200, 350);
    cv::imshow("Histogramme Egalisation", histogramToImageGS(computeHistogramGS(equalizeImage)));
    cv::moveWindow ("Histogramme Egalisation", 900, 500);
    cv::waitKey();

    // equalize exact the image
    cv::Mat equalizeExactImage = imageEqualizeExact(image);

    std::cout << "image size : " << equalizeExactImage.cols << " x " << equalizeExactImage.rows << std::endl;
    // display an image
    std::cout << "appuyer sur une touche ..." << std::endl;
    cv::imshow("Image Egalisation Exacte", equalizeExactImage);
    cv::moveWindow ("Image Egalisation Exacte", 200, 800);
    cv::imshow("Histogramme Egalisation Exacte", histogramToImageGS(computeHistogramGS(equalizeExactImage)));
    cv::moveWindow ("Histogramme Egalisation Exacte", 900, 800);
    cv::waitKey();

    return 1;
}



//attribuer les intensités
    // int intensite = 0;
    // int pas = (image.cols * image.rows) / 256;
    // for (int i = 0; i < (image.cols * image.rows); i++) {
    //     if (i % pas == 0) {
    //         intensite++;
    //     }
    //     newImage[i].z = intensite;
    // }